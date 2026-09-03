#include "test_help.h"
#include "ast-tool.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#    include <io.h>
#    define AST_DUP    _dup
#    define AST_DUP2    _dup2
#    define AST_FILENO _fileno
#    define AST_CLOSE  _close
#else
#    include <unistd.h>
#    define AST_DUP    dup
#    define AST_DUP2    dup2
#    define AST_FILENO fileno
#    define AST_CLOSE  close
#endif

namespace ast
{
namespace
{
    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    bool dispatch_command(std::vector<const char*> argv)
    {
        Arguments arguments;
        std::vector<std::u8string> args = args_to_utf8(static_cast<int32_t>(argv.size()), argv.data());
        if(!parse(arguments, args)) return false;
        return dispatch(arguments);
    }

    // Redirects stdout to a temp file for the duration of fn(), then restores it
    // and returns everything fn() printed.
    std::string capture_stdout(const std::function<void()>& fn)
    {
        std::filesystem::path tmp = std::filesystem::temp_directory_path() / "ast_tool_help_capture.txt";

        fflush(stdout);
        int saved_fd = AST_DUP(AST_FILENO(stdout));

        FILE* redirected = freopen(tmp.string().c_str(), "w", stdout);
        (void)redirected;

        fn();

        fflush(stdout);
        AST_DUP2(saved_fd, AST_FILENO(stdout));
        AST_CLOSE(saved_fd);

        std::ifstream in(tmp, std::ios::binary);
        std::string content{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{}};
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return content;
    }

    std::string top_level_help_text()
    {
        return capture_stdout([]() {
            dispatch_command({"ast-tool", "--help"});
        });
    }

    std::string command_help_text(const char* command)
    {
        return capture_stdout([&]() {
            dispatch_command({"ast-tool", command, "--help"});
        });
    }

    // -----------------------------------------------------------------------
    // 1. Top-level help: Primary commands are visible and appear before
    //    lower-priority groups.

    bool test_top_level_help_group_order()
    {
        bool ok = true;
        std::string text = top_level_help_text();

        size_t posPrimary = text.find("Primary");
        size_t posSecondary = text.find("Secondary");
        size_t posDebug = text.find("Debug");
        size_t posInfra = text.find("Infrastructure");

        ok &= check(posPrimary != std::string::npos, "top-level help shows a Primary group");
        ok &= check(posSecondary != std::string::npos, "top-level help shows a Secondary group");
        ok &= check(posDebug != std::string::npos, "top-level help shows a Debug group");
        ok &= check(posInfra != std::string::npos, "top-level help shows an Infrastructure group");

        if(ok) {
            ok &= check(posPrimary < posSecondary, "Primary group appears before Secondary");
            ok &= check(posSecondary < posDebug, "Secondary group appears before Debug");
            ok &= check(posDebug < posInfra, "Debug group appears before Infrastructure");
        }
        return ok;
    }

    bool test_top_level_help_primary_commands_visible()
    {
        std::string text = top_level_help_text();
        bool ok = true;
        for(const char* name : {"search", "callers", "references", "callees", "find", "symbols"}) {
            ok &= check(text.find(name) != std::string::npos, name);
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // 2. Category presentation: commands are listed under their expected group
    //    heading, using position-based assertions rather than exact formatting.

    bool command_appears_in_section(const std::string& text, const char* command,
                                     const char* section_start, const char* section_end)
    {
        size_t start = text.find(section_start);
        if(start == std::string::npos) return false;
        size_t end = section_end ? text.find(section_end, start + 1) : std::string::npos;
        size_t cmd_pos = text.find(std::string("\n    ") + command, start);
        return cmd_pos != std::string::npos && cmd_pos < end;
    }

    bool test_category_presentation()
    {
        std::string text = top_level_help_text();
        bool ok = true;

        for(const char* name : {"search", "callers", "references", "callees", "find", "symbols"}) {
            ok &= check(command_appears_in_section(text, name, "Primary", "Secondary"),
                        (std::string(name) + " appears in Primary").c_str());
        }
        ok &= check(command_appears_in_section(text, "outline", "Secondary", "Debug"),
                    "outline appears in Secondary");
        for(const char* name : {"parent", "children", "range"}) {
            ok &= check(command_appears_in_section(text, name, "Debug", "Infrastructure"),
                        (std::string(name) + " appears in Debug").c_str());
        }
        for(const char* name : {"setup", "cache"}) {
            ok &= check(command_appears_in_section(text, name, "Infrastructure", nullptr),
                        (std::string(name) + " appears in Infrastructure").c_str());
        }
        // Internal/debug-only command must not clutter the default surface.
        ok &= check(text.find("\n    dump") == std::string::npos,
                    "internal dump command is not listed in top-level help");
        return ok;
    }

    // -----------------------------------------------------------------------
    // 3. Direct invocation compatibility: representative commands from every
    //    group remain directly callable with harmless arguments.

    bool test_direct_invocation_compatibility()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "search", "data"}) == true,
                    "search remains directly callable");
        ok &= check(dispatch_command({"ast-tool", "find", "data/test00.cpp"}) == true,
                    "find remains directly callable");
        ok &= check(dispatch_command({"ast-tool", "outline", "data/test00.cpp"}) == true,
                    "outline remains directly callable");
        ok &= check(dispatch_command({"ast-tool", "parent", "--id", "9E52E360", "data/test00.cpp"}) == true,
                    "parent remains directly callable");
        ok &= check(dispatch_command({"ast-tool", "cache", "status", "data"}) == true,
                    "cache remains directly callable");
        return ok;
    }

    // -----------------------------------------------------------------------
    // 4. Command-specific help remains available even for less-prominent
    //    commands.

    bool test_command_specific_help_still_works()
    {
        bool ok = true;

        std::string search_help = command_help_text("search");
        ok &= check(search_help.find("search") != std::string::npos, "search --help prints its own help");

        std::string parent_help = command_help_text("parent");
        ok &= check(parent_help.find("parent") != std::string::npos,
                    "parent --help still works despite lower prominence");

        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_help()
{
    static const TestCase cases[] = {
        {"top-level help: group order",                 test_top_level_help_group_order},
        {"top-level help: primary commands visible",    test_top_level_help_primary_commands_visible},
        {"category presentation",                       test_category_presentation},
        {"direct invocation compatibility",              test_direct_invocation_compatibility},
        {"command-specific help still works",            test_command_specific_help_still_works},
    };

    std::cout << "=== help / discoverability tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
