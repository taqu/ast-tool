#include "test_setup.h"
#include "ast-tool.h"
#include "setup.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#else
#    include <unistd.h>
#endif

namespace ast
{
namespace
{

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool check(bool cond, const char* desc)
{
    if(!cond)
        std::cerr << "    FAIL: " << desc << "\n";
    return cond;
}

static std::string read_file(const std::filesystem::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if(!f) return {};
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>{}};
}

// Temporary directory that cleans itself up
struct TempDir
{
    std::filesystem::path path;
    TempDir() {
        std::error_code ec;
        path = std::filesystem::temp_directory_path(ec) / "ast_setup_test";
        // Make unique by appending pid
#if defined(_WIN32) || defined(_WIN64)
        path += std::to_string(GetCurrentProcessId());
#else
        path += std::to_string(getpid());
#endif
        static int counter = 0;
        path += "_";
        path += std::to_string(++counter);
        std::filesystem::remove_all(path, ec);
        std::filesystem::create_directories(path, ec);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// Build ArgSetup with overridden config paths stored in the strings below.
// We keep u8string storage alive for the duration of the test.
struct SetupOpts
{
    std::u8string claudeStr;
    std::u8string codexStr;
    ArgSetup args;

    SetupOpts(const std::filesystem::path& claudePath,
              const std::filesystem::path& codexPath)
    {
        claudeStr = reinterpret_cast<const char8_t*>(claudePath.string().c_str());
        codexStr  = reinterpret_cast<const char8_t*>(codexPath.string().c_str());
        args.claude_ = true;
        args.codex_  = true;
        args.dry_run_ = false;
        args.remove_  = false;
        args.claude_config_ = claudeStr.c_str();
        args.codex_config_  = codexStr.c_str();
    }
};

static bool contains(const std::string& hay, const std::string& needle)
{
    return hay.find(needle) != std::string::npos;
}

// ── Test 1: Fresh Claude config ───────────────────────────────────────────────

static bool test_claude_fresh()
{
    std::cout << "  test_claude_fresh\n";
    TempDir tmp;
    std::filesystem::path cfg = tmp.path / "claude" / "settings.json";
    SetupOpts opts(cfg, tmp.path / "codex_unused.toml");
    opts.args.codex_ = false;

    bool ok = run_setup(opts.args);
    if(!check(ok, "run_setup returned true")) return false;

    std::string content = read_file(cfg);
    bool pass = true;
    pass &= check(!content.empty(),                            "settings.json was created");
    pass &= check(contains(content, "SessionStart"),           "SessionStart key present");
    pass &= check(contains(content, "cache warm --background"), "cache warm command present");
    pass &= check(contains(content, "ast-tool-session-start-cache-warm"), "hook id present");
    return pass;
}

// ── Test 2: Existing Claude hooks are preserved ───────────────────────────────

static bool test_claude_existing_hooks_preserved()
{
    std::cout << "  test_claude_existing_hooks_preserved\n";
    TempDir tmp;
    std::filesystem::path cfg = tmp.path / "claude" / "settings.json";

    // Write a config with an existing user hook
    std::filesystem::create_directories(cfg.parent_path());
    {
        std::ofstream f(cfg);
        f << R"({
  "hooks": {
    "SessionStart": [
      {
        "hooks": [
          {
            "type": "command",
            "command": "user-hook-A"
          }
        ]
      }
    ]
  }
})";
    }

    SetupOpts opts(cfg, tmp.path / "unused.toml");
    opts.args.codex_ = false;
    bool ok = run_setup(opts.args);
    if(!check(ok, "run_setup returned true")) return false;

    std::string content = read_file(cfg);
    bool pass = true;
    pass &= check(contains(content, "user-hook-A"),            "existing user hook preserved");
    pass &= check(contains(content, "cache warm --background"), "ast-tool hook installed");
    return pass;
}

// ── Test 3: Idempotency ───────────────────────────────────────────────────────

static bool test_claude_idempotency()
{
    std::cout << "  test_claude_idempotency\n";
    TempDir tmp;
    std::filesystem::path cfg = tmp.path / "claude" / "settings.json";
    SetupOpts opts(cfg, tmp.path / "unused.toml");
    opts.args.codex_ = false;

    run_setup(opts.args);
    run_setup(opts.args);
    run_setup(opts.args);

    std::string content = read_file(cfg);
    // Count occurrences of kHookId
    size_t count = 0;
    size_t pos = 0;
    const std::string marker = "ast-tool-session-start-cache-warm";
    while((pos = content.find(marker, pos)) != std::string::npos) {
        ++count;
        pos += marker.size();
    }

    bool pass = check(count == 1, "exactly one ast-tool hook exists (idempotency)");
    return pass;
}

// ── Test 4: Fresh Codex config ────────────────────────────────────────────────

static bool test_codex_fresh()
{
    std::cout << "  test_codex_fresh\n";
    TempDir tmp;
    std::filesystem::path cfg = tmp.path / "codex" / "config.toml";
    SetupOpts opts(tmp.path / "unused.json", cfg);
    opts.args.claude_ = false;

    bool ok = run_setup(opts.args);
    if(!check(ok, "run_setup returned true")) return false;

    std::string content = read_file(cfg);
    bool pass = true;
    pass &= check(!content.empty(),                             "config.toml was created");
    pass &= check(contains(content, "[hooks]"),                 "[hooks] section present");
    pass &= check(contains(content, "session_start"),           "session_start key present");
    pass &= check(contains(content, "cache warm --background"), "cache warm command present");
    return pass;
}

// ── Test 5: Codex idempotency ─────────────────────────────────────────────────

static bool test_codex_idempotency()
{
    std::cout << "  test_codex_idempotency\n";
    TempDir tmp;
    std::filesystem::path cfg = tmp.path / "codex" / "config.toml";
    SetupOpts opts(tmp.path / "unused.json", cfg);
    opts.args.claude_ = false;

    run_setup(opts.args);
    run_setup(opts.args);
    run_setup(opts.args);

    std::string content = read_file(cfg);
    // Count occurrences of "cache warm"
    size_t count = 0;
    size_t pos = 0;
    while((pos = content.find("cache warm", pos)) != std::string::npos) {
        ++count;
        pos += 10;
    }

    bool pass = check(count == 1, "exactly one cache warm entry (Codex idempotency)");
    return pass;
}

// ── Test 6: Both agents configured together ───────────────────────────────────

static bool test_both_agents()
{
    std::cout << "  test_both_agents\n";
    TempDir tmp;
    std::filesystem::path claudeCfg = tmp.path / "claude" / "settings.json";
    std::filesystem::path codexCfg  = tmp.path / "codex"  / "config.toml";
    SetupOpts opts(claudeCfg, codexCfg);

    bool ok = run_setup(opts.args);
    if(!check(ok, "run_setup returned true")) return false;

    std::string claudeContent = read_file(claudeCfg);
    std::string codexContent  = read_file(codexCfg);

    bool pass = true;
    pass &= check(!claudeContent.empty(),                              "Claude settings.json created");
    pass &= check(contains(claudeContent, "cache warm --background"),  "Claude hook installed");
    pass &= check(!codexContent.empty(),                               "Codex config.toml created");
    pass &= check(contains(codexContent, "cache warm --background"),   "Codex hook installed");
    return pass;
}

// ── Test 7: Removal ───────────────────────────────────────────────────────────

static bool test_removal()
{
    std::cout << "  test_removal\n";
    TempDir tmp;

    // Write Claude config with user hook + ast-tool hook
    std::filesystem::path claudeCfg = tmp.path / "claude" / "settings.json";
    std::filesystem::create_directories(claudeCfg.parent_path());
    {
        std::ofstream f(claudeCfg);
        f << R"({
  "hooks": {
    "SessionStart": [
      {
        "hooks": [
          {
            "type": "command",
            "command": "user-hook-B"
          }
        ]
      },
      {
        "hooks": [
          {
            "type": "command",
            "command": "/fake/ast-tool cache warm --background",
            "id": "ast-tool-session-start-cache-warm"
          }
        ]
      }
    ]
  }
})";
    }

    std::filesystem::path codexCfg = tmp.path / "codex" / "config.toml";
    std::filesystem::create_directories(codexCfg.parent_path());
    {
        std::ofstream f(codexCfg);
        f << "[hooks]\n"
             "session_start = [\n"
             "  \"/fake/ast-tool cache warm --background\",\n"
             "  \"other-codex-hook\",\n"
             "]\n";
    }

    SetupOpts opts(claudeCfg, codexCfg);
    opts.args.remove_ = true;

    bool ok = run_setup(opts.args);
    if(!check(ok, "run_setup --remove returned true")) return false;

    std::string claudeContent = read_file(claudeCfg);
    std::string codexContent  = read_file(codexCfg);

    bool pass = true;
    pass &= check(contains(claudeContent, "user-hook-B"),             "Claude user hook preserved");
    pass &= check(!contains(claudeContent, "ast-tool-session-start"), "Claude ast-tool hook removed");
    pass &= check(contains(codexContent, "other-codex-hook"),         "Codex other hook preserved");
    pass &= check(!contains(codexContent, "ast-tool"),                "Codex ast-tool entry removed");
    return pass;
}

// ── Test 8: Dry run does not write files ──────────────────────────────────────

static bool test_dry_run()
{
    std::cout << "  test_dry_run\n";
    TempDir tmp;
    std::filesystem::path claudeCfg = tmp.path / "claude" / "settings.json";
    std::filesystem::path codexCfg  = tmp.path / "codex"  / "config.toml";
    SetupOpts opts(claudeCfg, codexCfg);
    opts.args.dry_run_ = true;

    bool ok = run_setup(opts.args);
    if(!check(ok, "dry run returned true")) return false;

    bool pass = true;
    pass &= check(!std::filesystem::exists(claudeCfg), "Claude settings.json NOT written on dry-run");
    pass &= check(!std::filesystem::exists(codexCfg),  "Codex config.toml NOT written on dry-run");
    return pass;
}

} // namespace

// ── Entry point ───────────────────────────────────────────────────────────────

bool run_tests_setup()
{
    std::cout << "-----------------------------------------------------\n";
    std::cout << "run_tests_setup\n";

    bool ok = true;
    ok &= test_claude_fresh();
    ok &= test_claude_existing_hooks_preserved();
    ok &= test_claude_idempotency();
    ok &= test_codex_fresh();
    ok &= test_codex_idempotency();
    ok &= test_both_agents();
    ok &= test_removal();
    ok &= test_dry_run();

    std::cout << (ok ? "  PASS" : "  FAIL") << "\n";
    return ok;
}

} // namespace ast
