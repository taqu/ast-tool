#include "ast-tool.h"
#include "setup.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#    define SETUP_WINDOWS
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#    if defined(__APPLE__)
#        include <mach-o/dyld.h>
#    endif
#endif

namespace ast
{
bool parse_setup(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    // Syntax:
    //   ast-tool setup [--claude] [--codex] [--all] [--global] [--dry-run] [--remove]

    arguments.sub_ = SubCommand::Setup;

    if(argc < 2) {
        return false;
    }
    ArgSetup& args = arguments.setup_;
    arguments.sub_ = SubCommand::Setup;
    args.claude_ = true;
    args.codex_ = true;
    args.dry_run_ = false;
    args.remove_ = false;
    args.global_ = false;
    args.local_ = false;
    args.claude_config_ = nullptr;
    args.codex_config_ = nullptr;
    bool only_one = false;
    for(int32_t i = 2; i < argc; ++i) {
        std::u8string_view flag{argv[i]};
        if(flag == u8"--claude") {
            if(!only_one) {
                args.codex_ = false;
                only_one = true;
            }
            args.claude_ = true;
        } else if(flag == u8"--codex") {
            if(!only_one) {
                args.claude_ = false;
                only_one = true;
            }
            args.codex_ = true;
        } else if(flag == u8"--all") {
            args.claude_ = true;
            args.codex_ = true;
        }else if(flag == u8"--global"){
            args.global_ = true;
        }else if(flag == u8"--local"){
            args.local_ = true;
        } else if(flag == u8"--dry-run"){
            args.dry_run_ = true;
        }else if(flag == u8"--remove"){
            args.remove_ = true;
        }
    }
    arguments.sub_ = SubCommand::Setup;

    return true;
}

// ── Executable path ───────────────────────────────────────────────────────────

std::filesystem::path get_executable_path()
{
#if defined(SETUP_WINDOWS)
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if(len == 0 || len == MAX_PATH)
        return {};
    return std::filesystem::path(buf);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if(_NSGetExecutablePath(buf.data(), &size) != 0)
        return {};
    return std::filesystem::canonical(buf);
#else
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if(len < 0)
        return {};
    buf[len] = '\0';
    return std::filesystem::path(buf);
#endif
}

// ── Background process spawn ──────────────────────────────────────────────────

bool spawn_background(const std::filesystem::path& exe,
                      const std::vector<std::string>& args)
{
#if defined(SETUP_WINDOWS)
    std::string cmdline;
    {
        std::string e = exe.string();
        cmdline += '"';
        for(char c : e) {
            if(c == '"') cmdline += "\\\"";
            else         cmdline += c;
        }
        cmdline += '"';
    }
    for(const auto& a : args) {
        cmdline += " \"";
        for(char c : a) {
            if(c == '"') cmdline += "\\\"";
            else         cmdline += c;
        }
        cmdline += '"';
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessA(
        nullptr,
        cmdline.data(),
        nullptr, nullptr,
        FALSE,
        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);

    if(ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
#else
    pid_t pid = fork();
    if(pid < 0)
        return false;
    if(pid > 0)
        return true; // parent returns immediately

    // Child: detach and exec
    setsid();

    int devnull = open("/dev/null", O_RDWR);
    if(devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if(devnull > STDERR_FILENO)
            close(devnull);
    }

    std::string exeStr = exe.string();
    std::vector<const char*> argv_ptrs;
    argv_ptrs.push_back(exeStr.c_str());
    for(const auto& a : args)
        argv_ptrs.push_back(a.c_str());
    argv_ptrs.push_back(nullptr);

    execv(exeStr.c_str(), const_cast<char* const*>(argv_ptrs.data()));
    _exit(1);
#endif
}

// ── File I/O helpers ──────────────────────────────────────────────────────────

static std::string read_file(const std::filesystem::path& path)
{
    FILE* f = nullptr;
#if defined(SETUP_WINDOWS)
    errno_t err = fopen_s(&f, path.string().c_str(), "rb");
    if(err != 0 || !f)
        return {};
#else
    f = fopen(path.string().c_str(), "rb");
    if(!f)
        return {};
#endif
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string s;
    if(sz > 0) {
        s.resize(sz);
        (void)fread(s.data(), 1, (size_t)sz, f);
    }
    fclose(f);
    return s;
}

static bool atomic_write(const std::filesystem::path& target, const std::string& content)
{
    std::filesystem::path tmp = target.string() + ".tmp";

    FILE* f = nullptr;
#if defined(SETUP_WINDOWS)
    errno_t err = fopen_s(&f, tmp.string().c_str(), "wb");
    if(err != 0 || !f)
        return false;
#else
    f = fopen(tmp.string().c_str(), "wb");
    if(!f)
        return false;
#endif

    bool ok = (fwrite(content.data(), 1, content.size(), f) == content.size());
    fflush(f);
    fclose(f);

    if(!ok) {
        std::error_code ec2;
        std::filesystem::remove(tmp, ec2);
        return false;
    }

    std::error_code ec;
    std::filesystem::rename(tmp, target, ec);
    if(ec) {
        std::error_code ec2;
        std::filesystem::remove(tmp, ec2);
        return false;
    }
    return true;
}

// ── Hook identity marker ──────────────────────────────────────────────────────

static constexpr const char* kHookId = "ast-tool-session-start-cache-warm";

// ── Minimal JSON value ────────────────────────────────────────────────────────

enum class JType { Null, Bool, Number, String, Array, Object };

struct JVal
{
    JType type = JType::Null;
    bool b = false;
    std::string s;
    std::vector<JVal> arr;
    std::vector<std::pair<std::string, JVal>> obj;

    bool is_null()   const { return type == JType::Null; }
    bool is_object() const { return type == JType::Object; }
    bool is_array()  const { return type == JType::Array; }
    bool is_string() const { return type == JType::String; }

    JVal* get(const std::string& key) {
        for(auto& kv : obj)
            if(kv.first == key) return &kv.second;
        return nullptr;
    }
    const JVal* get(const std::string& key) const {
        for(const auto& kv : obj)
            if(kv.first == key) return &kv.second;
        return nullptr;
    }
    void set(std::string key, JVal val) {
        for(auto& kv : obj)
            if(kv.first == key) { kv.second = std::move(val); return; }
        obj.emplace_back(std::move(key), std::move(val));
    }
    void erase(const std::string& key) {
        for(auto it = obj.begin(); it != obj.end(); ++it)
            if(it->first == key) { obj.erase(it); return; }
    }

    static JVal null_v()  { return {}; }
    static JVal bool_v(bool v) { JVal j; j.type = JType::Bool; j.b = v; return j; }
    static JVal str_v(std::string v)  { JVal j; j.type = JType::String; j.s = std::move(v); return j; }
    static JVal arr_v()  { JVal j; j.type = JType::Array;  return j; }
    static JVal obj_v()  { JVal j; j.type = JType::Object; return j; }
};

// ── JSON parser ───────────────────────────────────────────────────────────────

struct JParser
{
    const char* p;
    const char* end;

    void skip_ws() {
        while(p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
            ++p;
    }

    std::string parse_string() {
        if(p >= end || *p != '"') return {};
        ++p;
        std::string s;
        while(p < end && *p != '"') {
            if(*p == '\\') {
                ++p;
                if(p >= end) break;
                switch(*p) {
                case '"':  s += '"';  break;
                case '\\': s += '\\'; break;
                case '/':  s += '/';  break;
                case 'n':  s += '\n'; break;
                case 'r':  s += '\r'; break;
                case 't':  s += '\t'; break;
                case 'b':  s += '\b'; break;
                case 'f':  s += '\f'; break;
                case 'u':
                    // Skip 4 hex digits; pass through as-is (best-effort)
                    for(int i = 0; i < 4 && p + 1 < end; ++i) ++p;
                    s += '?';
                    break;
                default:   s += *p;   break;
                }
                ++p;
            } else {
                s += *p++;
            }
        }
        if(p < end) ++p; // closing "
        return s;
    }

    JVal parse_value() {
        skip_ws();
        if(p >= end) return {};
        if(*p == '"') {
            return JVal::str_v(parse_string());
        }
        if(*p == '{') {
            ++p;
            JVal v = JVal::obj_v();
            skip_ws();
            if(p < end && *p == '}') { ++p; return v; }
            while(p < end) {
                skip_ws();
                std::string key = parse_string();
                skip_ws();
                if(p < end && *p == ':') ++p;
                JVal val = parse_value();
                v.obj.emplace_back(std::move(key), std::move(val));
                skip_ws();
                if(p < end && *p == ',') { ++p; continue; }
                break;
            }
            skip_ws();
            if(p < end && *p == '}') ++p;
            return v;
        }
        if(*p == '[') {
            ++p;
            JVal v = JVal::arr_v();
            skip_ws();
            if(p < end && *p == ']') { ++p; return v; }
            while(p < end) {
                v.arr.push_back(parse_value());
                skip_ws();
                if(p < end && *p == ',') { ++p; continue; }
                break;
            }
            skip_ws();
            if(p < end && *p == ']') ++p;
            return v;
        }
        if(p + 4 <= end && strncmp(p, "true",  4) == 0) { p += 4; return JVal::bool_v(true); }
        if(p + 5 <= end && strncmp(p, "false", 5) == 0) { p += 5; return JVal::bool_v(false); }
        if(p + 4 <= end && strncmp(p, "null",  4) == 0) { p += 4; return {}; }
        // Number (stored as raw string)
        JVal v; v.type = JType::Number;
        const char* start = p;
        if(p < end && *p == '-') ++p;
        while(p < end && *p >= '0' && *p <= '9') ++p;
        if(p < end && *p == '.') { ++p; while(p < end && *p >= '0' && *p <= '9') ++p; }
        if(p < end && (*p == 'e' || *p == 'E')) {
            ++p;
            if(p < end && (*p == '+' || *p == '-')) ++p;
            while(p < end && *p >= '0' && *p <= '9') ++p;
        }
        v.s = std::string(start, p);
        return v;
    }
};

static JVal json_parse(const std::string& src)
{
    JParser parser{src.data(), src.data() + src.size()};
    return parser.parse_value();
}

// ── JSON serializer ───────────────────────────────────────────────────────────

static std::string json_escape(const std::string& s)
{
    std::string r;
    r.reserve(s.size() + 4);
    for(unsigned char c : s) {
        if(c == '"')       r += "\\\"";
        else if(c == '\\') r += "\\\\";
        else if(c == '\n') r += "\\n";
        else if(c == '\r') r += "\\r";
        else if(c == '\t') r += "\\t";
        else if(c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); r += buf; }
        else               r += (char)c;
    }
    return r;
}

static void json_serialize(const JVal& v, std::string& out, int depth)
{
    std::string pad(depth * 2, ' ');
    std::string inner((depth + 1) * 2, ' ');
    switch(v.type) {
    case JType::Null:   out += "null";  break;
    case JType::Bool:   out += v.b ? "true" : "false"; break;
    case JType::Number: out += v.s;     break;
    case JType::String:
        out += '"'; out += json_escape(v.s); out += '"';
        break;
    case JType::Array:
        if(v.arr.empty()) { out += "[]"; break; }
        out += "[\n";
        for(size_t i = 0; i < v.arr.size(); ++i) {
            out += inner;
            json_serialize(v.arr[i], out, depth + 1);
            if(i + 1 < v.arr.size()) out += ',';
            out += '\n';
        }
        out += pad; out += ']';
        break;
    case JType::Object:
        if(v.obj.empty()) { out += "{}"; break; }
        out += "{\n";
        for(size_t i = 0; i < v.obj.size(); ++i) {
            out += inner;
            out += '"'; out += json_escape(v.obj[i].first); out += "\": ";
            json_serialize(v.obj[i].second, out, depth + 1);
            if(i + 1 < v.obj.size()) out += ',';
            out += '\n';
        }
        out += pad; out += '}';
        break;
    }
}

static std::string json_to_string(const JVal& v)
{
    std::string out;
    json_serialize(v, out, 0);
    out += '\n';
    return out;
}

// ── Default config paths ──────────────────────────────────────────────────────

static std::filesystem::path current_dir()
{
    return std::filesystem::current_path();
}

static std::filesystem::path home_dir(const char* home_env)
{
    assert(nullptr != home_env);
    const char* home = getenv(home_env);
    if(!home) {
#if defined(SETUP_WINDOWS)
        home = getenv("USERPROFILE");
        if(!home) {
            home = getenv("HOMEDRIVE");
        }
#else
        home = getenv("HOME");
#endif
    }
    return home? std::filesystem::path(home) : std::filesystem::path{};
}

static std::filesystem::path default_claude_config(bool global, bool local)
{
    std::filesystem::path h;
    const char* filename = nullptr;
    if(global){
        h = home_dir("CLAUDE_HOME");
        filename = "settings.json";
    }else{
        h = current_dir();
        filename = local? "settings.local.json" : "settings.json";
    }
    return h.empty() ? std::filesystem::path{} : h / ".claude" / filename;
}

static std::filesystem::path default_codex_config(bool global, bool local)
{
    std::filesystem::path h;
    const char* filename = nullptr;
    if(global){
        h = home_dir("CODEX_HOME");
        filename = "config.toml";
    }else{
        h = current_dir();
        filename = local? "config.local.toml" : "config.toml";
    }
    return h.empty() ? std::filesystem::path{} : h / ".claude" / filename;
}

// ── Setup result ──────────────────────────────────────────────────────────────

enum class SetupResult { Installed, AlreadyConfigured, Updated, Removed, Failed };

// ── Claude Code setup (settings.json) ────────────────────────────────────────
//
// Hook structure:
//   {
//     "hooks": {
//       "SessionStart": [
//         {
//           "hooks": [
//             { "type": "command", "command": "<exe> cache warm --background",
//               "id": "ast-tool-session-start-cache-warm" }
//           ]
//         }
//       ]
//     }
//   }
//
// Our hook is identified by "id" == kHookId on the inner hook object.

static SetupResult setup_claude(const std::filesystem::path& configPath,
                                const std::filesystem::path& exePath,
                                bool dry_run, bool remove_hook)
{
    std::string warmCmd = exePath.string() + " cache warm --background";

    // Read and parse existing config
    std::string existing;
    if(std::filesystem::exists(configPath))
        existing = read_file(configPath);

    JVal root;
    if(!existing.empty()) {
        root = json_parse(existing);
        if(!root.is_object())
            root = JVal::obj_v();
    } else {
        root = JVal::obj_v();
    }

    // Ensure hooks object
    if(!root.get("hooks") || !root.get("hooks")->is_object())
        root.set("hooks", JVal::obj_v());
    JVal& hooks = *root.get("hooks");

    // Ensure SessionStart array
    if(!hooks.get("SessionStart") || !hooks.get("SessionStart")->is_array())
        hooks.set("SessionStart", JVal::arr_v());
    JVal& ss = *hooks.get("SessionStart");

    // Search for our hook across all groups
    auto is_our_hook = [&](const JVal& hook) -> bool {
        if(!hook.is_object()) return false;
        const JVal* idv = hook.get("id");
        if(idv && idv->is_string() && idv->s == kHookId) return true;
        // Fallback: match by command content
        const JVal* cmdv = hook.get("command");
        if(cmdv && cmdv->is_string()) {
            const std::string& cmd = cmdv->s;
            return cmd.find("ast-tool") != std::string::npos &&
                   cmd.find("cache warm") != std::string::npos;
        }
        return false;
    };

    bool found = false;
    bool changed = false;

    for(auto& group : ss.arr) {
        if(!group.is_object()) continue;
        JVal* groupHooks = group.get("hooks");
        if(!groupHooks || !groupHooks->is_array()) continue;

        for(size_t i = 0; i < groupHooks->arr.size(); ++i) {
            if(!is_our_hook(groupHooks->arr[i])) continue;
            found = true;

            if(remove_hook) {
                groupHooks->arr.erase(groupHooks->arr.begin() + (ptrdiff_t)i);
                changed = true;
                // Clean up empty group
                if(groupHooks->arr.empty()) {
                    for(size_t gi = 0; gi < ss.arr.size(); ++gi) {
                        if(&ss.arr[gi] == &group) {
                            ss.arr.erase(ss.arr.begin() + (ptrdiff_t)gi);
                            break;
                        }
                    }
                }
            } else {
                // Update command/id if stale
                JVal& hook = groupHooks->arr[i];
                const JVal* cmdv = hook.get("command");
                const JVal* idv  = hook.get("id");
                if(!cmdv || !cmdv->is_string() || cmdv->s != warmCmd) {
                    hook.set("command", JVal::str_v(warmCmd));
                    changed = true;
                }
                if(!idv || !idv->is_string() || idv->s != kHookId) {
                    hook.set("id", JVal::str_v(kHookId));
                    changed = true;
                }
            }
            break; // only one of ours
        }
        if(found) break;
    }

    if(remove_hook) {
        if(!found) return SetupResult::AlreadyConfigured;
        if(dry_run) return SetupResult::Removed;
        std::error_code ec;
        std::filesystem::create_directories(configPath.parent_path(), ec);
        return atomic_write(configPath, json_to_string(root)) ? SetupResult::Removed : SetupResult::Failed;
    }

    if(found && !changed)
        return SetupResult::AlreadyConfigured;

    if(!found) {
        // Build new hook entry
        JVal hookEntry = JVal::obj_v();
        hookEntry.set("type",    JVal::str_v("command"));
        hookEntry.set("command", JVal::str_v(warmCmd));
        hookEntry.set("id",      JVal::str_v(kHookId));

        JVal groupHooksArr = JVal::arr_v();
        groupHooksArr.arr.push_back(std::move(hookEntry));

        JVal group = JVal::obj_v();
        group.set("hooks", std::move(groupHooksArr));

        ss.arr.push_back(std::move(group));
    }

    if(dry_run)
        return found ? SetupResult::Updated : SetupResult::Installed;

    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);
    if(ec) return SetupResult::Failed;

    return atomic_write(configPath, json_to_string(root))
        ? (found ? SetupResult::Updated : SetupResult::Installed)
        : SetupResult::Failed;
}

// ── Codex setup (config.toml) ─────────────────────────────────────────────────
//
// Target structure:
//   [hooks]
//   session_start = [
//     "/path/to/ast-tool cache warm --background",
//   ]
//
// Our entry is identified by containing "ast-tool" and "cache warm".
// The parser handles: missing file, missing [hooks] section,
// missing session_start key, single-string value, single-line array,
// and multi-line array (closing ] on its own line).

namespace
{

static std::string toml_escape(const std::string& s)
{
    std::string r;
    for(char c : s) {
        if(c == '"')  r += "\\\"";
        else if(c == '\\') r += "\\\\";
        else r += c;
    }
    return r;
}

static std::string trim_str(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Parse a TOML basic string starting at pos (pointing at opening ").
// Advances pos past the closing ".
static std::string parse_toml_str(const std::string& line, size_t& pos)
{
    if(pos >= line.size() || line[pos] != '"') return {};
    ++pos;
    std::string r;
    while(pos < line.size() && line[pos] != '"') {
        if(line[pos] == '\\' && pos + 1 < line.size()) {
            ++pos;
            switch(line[pos]) {
            case 'n':  r += '\n'; break;
            case 't':  r += '\t'; break;
            case '"':  r += '"';  break;
            case '\\': r += '\\'; break;
            default:   r += line[pos]; break;
            }
        } else {
            r += line[pos];
        }
        ++pos;
    }
    if(pos < line.size()) ++pos; // closing "
    return r;
}

} // anonymous namespace

static SetupResult setup_codex(const std::filesystem::path& configPath,
                               const std::filesystem::path& exePath,
                               bool dry_run, bool remove_hook)
{
    std::string warmCmd = exePath.string() + " cache warm --background";

    std::string existing;
    if(std::filesystem::exists(configPath))
        existing = read_file(configPath);

    // Split into lines
    std::vector<std::string> lines;
    {
        std::istringstream iss(existing);
        std::string l;
        while(std::getline(iss, l)) {
            // Strip trailing \r
            if(!l.empty() && l.back() == '\r') l.pop_back();
            lines.push_back(l);
        }
    }

    // Locate [hooks] section and session_start key
    int hooksSectionLine = -1; // line index of [hooks]
    int ssStartLine      = -1; // line of "session_start = ..."
    int ssEndLine        = -1; // last line of the value (inclusive)
    std::vector<std::string> ssValues; // decoded string values

    bool inHooks = false;
    bool inSSArray = false; // parsing multi-line array

    for(int i = 0; i < (int)lines.size(); ++i) {
        const std::string& raw = lines[i];
        std::string t = trim_str(raw);

        if(inSSArray) {
            if(t == "]" || t == "],") {
                ssEndLine = i;
                inSSArray = false;
            } else {
                size_t pos = 0;
                while(pos < t.size() && (t[pos] == ' ' || t[pos] == '\t')) ++pos;
                if(pos < t.size() && t[pos] == '"') {
                    ssValues.push_back(parse_toml_str(t, pos));
                }
                // ignore comment lines (#) and trailing commas
            }
            continue;
        }

        if(!t.empty() && t[0] == '[') {
            if(t == "[hooks]") {
                inHooks = true;
                hooksSectionLine = i;
            } else {
                // Any other section ends [hooks] scope
                inHooks = false;
            }
            continue;
        }

        if(inHooks && ssStartLine < 0) {
            size_t eq = t.find('=');
            if(eq == std::string::npos) continue;
            std::string key = trim_str(t.substr(0, eq));
            if(key != "session_start") continue;

            ssStartLine = i;
            std::string val = trim_str(t.substr(eq + 1));

            if(!val.empty() && val[0] == '[') {
                // Array
                size_t close = val.rfind(']');
                if(close != std::string::npos) {
                    // Single-line array
                    ssEndLine = i;
                    size_t pos = 1;
                    while(pos < close) {
                        while(pos < close && (val[pos] == ' ' || val[pos] == '\t' || val[pos] == ',')) ++pos;
                        if(pos < close && val[pos] == '"') {
                            ssValues.push_back(parse_toml_str(val, pos));
                        } else if(pos < close && val[pos] == '#') {
                            break;
                        } else if(pos < close) {
                            ++pos;
                        }
                    }
                } else {
                    // Multi-line array — collect subsequent lines
                    inSSArray = true;
                    // ssEndLine will be set when we find the closing ]
                }
            } else if(!val.empty() && val[0] == '"') {
                // Single string value
                size_t pos = 0;
                ssValues.push_back(parse_toml_str(val, pos));
                ssEndLine = i;
            }
        }
    }

    // Malformed TOML: multi-line array was never closed
    if(ssStartLine >= 0 && ssEndLine < 0) {
        ssStartLine = -1;
        ssValues.clear();
    }

    // Find our entry in ssValues
    int ourIdx = -1;
    for(int i = 0; i < (int)ssValues.size(); ++i) {
        const std::string& v = ssValues[i];
        if(v.find("ast-tool") != std::string::npos &&
           v.find("cache warm") != std::string::npos) {
            ourIdx = i;
            break;
        }
    }

    bool found = (ourIdx >= 0);
    bool upToDate = found && (ssValues[ourIdx] == warmCmd);

    if(remove_hook) {
        if(!found) return SetupResult::AlreadyConfigured;
        ssValues.erase(ssValues.begin() + ourIdx);
    } else {
        if(found && upToDate) return SetupResult::AlreadyConfigured;
        if(found) ssValues[ourIdx] = warmCmd;
        else      ssValues.push_back(warmCmd);
    }

    if(dry_run)
        return found ? (remove_hook ? SetupResult::Removed : SetupResult::Updated)
                     : SetupResult::Installed;

    // Build the new session_start block (always multi-line array)
    std::vector<std::string> ssBlock;
    ssBlock.push_back("session_start = [");
    for(const auto& v : ssValues)
        ssBlock.push_back("  \"" + toml_escape(v) + "\",");
    ssBlock.push_back("]");

    // Reconstruct the file
    std::vector<std::string> newLines;

    if(hooksSectionLine < 0) {
        // No [hooks] section — append
        newLines = lines;
        if(!newLines.empty() && !trim_str(newLines.back()).empty())
            newLines.push_back("");
        newLines.push_back("[hooks]");
        for(const auto& l : ssBlock) newLines.push_back(l);
    } else if(ssStartLine < 0) {
        // [hooks] exists but no session_start — insert right after [hooks] line
        for(int i = 0; i < (int)lines.size(); ++i) {
            newLines.push_back(lines[i]);
            if(i == hooksSectionLine) {
                for(const auto& l : ssBlock) newLines.push_back(l);
            }
        }
    } else {
        // Replace the session_start block (ssStartLine..ssEndLine)
        for(int i = 0; i < (int)lines.size(); ++i) {
            if(i == ssStartLine) {
                for(const auto& l : ssBlock) newLines.push_back(l);
                i = ssEndLine; // skip through end of old block
            } else {
                newLines.push_back(lines[i]);
            }
        }
    }

    std::string output;
    output.reserve(newLines.size() * 40);
    for(const auto& l : newLines) {
        output += l;
        output += '\n';
    }

    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);
    if(ec) return SetupResult::Failed;

    if(!atomic_write(configPath, output))
        return SetupResult::Failed;

    return found ? (remove_hook ? SetupResult::Removed : SetupResult::Updated)
                 : SetupResult::Installed;
}

// ── run_setup ─────────────────────────────────────────────────────────────────

bool run_setup(const ArgSetup& args)
{
    std::filesystem::path exePath = get_executable_path();
    if(exePath.empty()) {
        fprintf(stderr, "ast-tool setup: warning: could not determine executable path\n");
        exePath = "ast-tool";
    }

    std::filesystem::path claudePath = args.claude_config_
        ? std::filesystem::path(reinterpret_cast<const char*>(args.claude_config_))
        : default_claude_config(args.global_, args.local_);

    std::filesystem::path codexPath = args.codex_config_
        ? std::filesystem::path(reinterpret_cast<const char*>(args.codex_config_))
        : default_codex_config(args.global_, args.local_);

    bool overall_ok = true;

    auto print_result = [&](const char* agent, const char* hookLabel, SetupResult r,
                            const std::filesystem::path& cfgPath) {
        switch(r) {
        case SetupResult::Installed:
            fprintf(stdout, "  %s: installed%s\n", hookLabel, args.dry_run_ ? " (dry run)" : "");
            break;
        case SetupResult::AlreadyConfigured:
            fprintf(stdout, "  %s: %s\n", hookLabel,
                    args.remove_ ? "not found" : "already configured");
            break;
        case SetupResult::Updated:
            fprintf(stdout, "  %s: updated%s\n", hookLabel, args.dry_run_ ? " (dry run)" : "");
            break;
        case SetupResult::Removed:
            fprintf(stdout, "  %s: removed%s\n", hookLabel, args.dry_run_ ? " (dry run)" : "");
            break;
        case SetupResult::Failed:
            fprintf(stdout, "  %s: FAILED\n", hookLabel);
            fprintf(stderr, "  %s configuration: %s\n", agent, cfgPath.string().c_str());
            overall_ok = false;
            break;
        }
    };

    if(args.claude_) {
        fprintf(stdout, "Claude Code:\n");
        if(claudePath.empty()) {
            fprintf(stdout, "  not detected (HOME not set)\n");
        } else {
            auto r = setup_claude(claudePath, exePath, args.dry_run_, args.remove_);
            print_result("Claude Code", "SessionStart hook", r, claudePath);
        }
    }

    if(args.codex_) {
        fprintf(stdout, "Codex:\n");
        if(codexPath.empty()) {
            fprintf(stdout, "  not detected (HOME not set)\n");
        } else {
            auto r = setup_codex(codexPath, exePath, args.dry_run_, args.remove_);
            print_result("Codex", "session_start hook", r, codexPath);
        }
    }

    if(!args.remove_ && !args.dry_run_ && overall_ok &&
       (args.claude_ || args.codex_)) {
        fprintf(stdout, "\nAST cache warming will run automatically at session start.\n");
    }

    return overall_ok;
}

} // namespace ast
