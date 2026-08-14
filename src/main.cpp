#include "ast-tool.h"
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>

namespace
{
    std::optional<std::string> wide_to_utf8(const wchar_t* wide)
    {
        if(!wide) return std::string{};
        int needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if(needed <= 0) return std::nullopt;
        std::string result(static_cast<size_t>(needed - 1), '\0');
        int written = WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), needed, nullptr, nullptr);
        if(written <= 0) return std::nullopt;
        return result;
    }
} // namespace

int wmain(int argc, wchar_t** argv)
{
    try {
        ast::initialize();

        std::vector<std::u8string> args;
        args.reserve(static_cast<size_t>(argc));
        for(int i = 0; i < argc; ++i) {
            std::optional<std::string> utf8 = wide_to_utf8(argv[i]);
            if(!utf8) {
                fprintf(stderr, "error: failed to convert command-line argument to UTF-8\n");
                return 1;
            }
            args.emplace_back(reinterpret_cast<const char8_t*>(utf8->data()), utf8->size());
        }

        ast::Arguments arguments;
        if(!ast::parse(arguments, args)) {
            fprintf(stderr, "error: unknown command or missing required arguments\n");
            fprintf(stderr, "Run 'ast-tool help' to see available commands.\n");
            return 1;
        }
        return ast::dispatch(arguments) ? 0 : 1;
    } catch(const std::exception& e) {
        fprintf(stderr, "error: unexpected exception: %s\n", e.what());
        return 1;
    } catch(...) {
        fprintf(stderr, "error: unexpected exception\n");
        return 1;
    }
}

#else

int main(int argc, char** argv)
{
    try {
        ast::initialize();

        std::vector<std::u8string> args;
        args.reserve(static_cast<size_t>(argc));
        for(int i = 0; i < argc; ++i) {
            // POSIX argv is the UTF-8 encoding boundary; reinterpret is required here.
            args.emplace_back(reinterpret_cast<const char8_t*>(argv[i]));
        }

        ast::Arguments arguments;
        if(!ast::parse(arguments, args)) {
            fprintf(stderr, "error: unknown command or missing required arguments\n");
            fprintf(stderr, "Run 'ast-tool help' to see available commands.\n");
            return 1;
        }
        return ast::dispatch(arguments) ? 0 : 1;
    } catch(const std::exception& e) {
        fprintf(stderr, "error: unexpected exception: %s\n", e.what());
        return 1;
    } catch(...) {
        fprintf(stderr, "error: unexpected exception\n");
        return 1;
    }
}

#endif
