#include "cache.h"
#include "ast-cache-db.h"
#include "ast-cache.h"
#include "ast-tool.h"
#include "cache-warm.h"
#include "setup.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ast
{

bool parse_cache(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    // Syntax:
    //   ast-tool cache warm  [--verbose] [<root>]
    //   ast-tool cache status [<root>]
    //
    // argv[0] = "ast-tool", argv[1] = "cache", argv[2] = "warm"|"status", ...

    arguments.sub_ = SubCommand::None;

    if(argc < 3) {
        // No subcommand: treat as help.
        arguments.sub_ = SubCommand::Help;
        arguments.help_.topic_ = argv[1]; // "cache"
        return true;
    }

    std::u8string_view sub{argv[2]};
    bool isWarm   = (sub == u8"warm");
    bool isStatus = (sub == u8"status");

    if(!isWarm && !isStatus) {
        arguments.sub_ = SubCommand::Help;
        arguments.help_.topic_ = argv[1];
        return true;
    }

    ArgCache& a = arguments.cache_;
    a.warm_    = isWarm;
    a.status_  = isStatus;
    a.verbose_ = false;
    a.root_    = nullptr;

    a.background_ = false;
    for(int32_t i = 3; i < argc; ++i) {
        std::u8string_view flag{argv[i]};
        if(flag == u8"--verbose" || flag == u8"-v") {
            a.verbose_ = true;
        } else if(flag == u8"--background") {
            a.background_ = true;
        } else if(flag.starts_with(u8"-")) {
            // Unknown flag — ignore.
        } else {
            a.root_ = argv[i];
        }
    }

    // Default root to current directory.
    if(!a.root_)
        a.root_ = u8".";

    arguments.sub_ = isWarm ? SubCommand::CacheWarm : SubCommand::CacheStatus;
    return true;
}

bool cache_warm_cmd(const ArgCache& args)
{
    if(args.background_) {
        // Spawn a detached background process running cache warm (without --background)
        std::filesystem::path exe = get_executable_path();
        if(exe.empty()) exe = "ast-tool";
        std::vector<std::string> bgArgs = {"cache", "warm"};
        if(args.verbose_) bgArgs.emplace_back("--verbose");
        if(args.root_)    bgArgs.emplace_back(reinterpret_cast<const char*>(args.root_));
        return spawn_background(exe, bgArgs);
    }

    std::filesystem::path root(args.root_);
    WarmStats stats;
    WarmResult result = warm_cache(root, stats, args.verbose_);

    switch(result) {
    case WarmResult::Success:
        return true;
    case WarmResult::LockBusy:
        // Another warmer is active — exit silently (best-effort, non-blocking).
        return true;
    case WarmResult::DatabaseError:
        fprintf(stderr, "ast-tool cache warm: failed to open cache database\n");
        return false;
    case WarmResult::ScanError:
        fprintf(stderr, "ast-tool cache warm: failed to scan workspace '%s'\n",
                (const char*)args.root_);
        return false;
    }
    return false;
}

bool cache_status_cmd(const ArgCache& args)
{
    std::filesystem::path root(args.root_);
    std::filesystem::path cacheDir = root / ".ast-tool";
    std::filesystem::path dbPath   = cacheDir / "ast-cache.db";

    ASTCacheDatabase db;
    if(!db.open(dbPath)) {
        fprintf(stdout, "Cache database: not found\n");
        fprintf(stdout, "  Path: %s\n", dbPath.string().c_str());
        return true;
    }

    int64_t count   = db.entry_count();
    int64_t dbBytes = db.db_size_bytes(dbPath);

    fprintf(stdout, "Cache database:      %s\n", dbPath.string().c_str());
    fprintf(stdout, "AST format version:  %u\n", kAstCacheFormatVersion);
    fprintf(stdout, "Cached files:        %lld\n", (long long)count);
    if(dbBytes >= 0) {
        if(dbBytes >= 1024 * 1024)
            fprintf(stdout, "Database size:       %.2f MB\n", (double)dbBytes / (1024.0 * 1024.0));
        else if(dbBytes >= 1024)
            fprintf(stdout, "Database size:       %.2f KB\n", (double)dbBytes / 1024.0);
        else
            fprintf(stdout, "Database size:       %lld bytes\n", (long long)dbBytes);
    }
    return true;
}

} // namespace ast
