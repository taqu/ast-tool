#ifndef INC_AST_SETUP_H_
#define INC_AST_SETUP_H_

#include <filesystem>
#include <string>
#include <vector>

namespace ast
{

// ArgSetup is declared in ast-tool.h (include/ast-tool.h).
// Include ast-tool.h before setup.h to see the full definition.
struct ArgSetup;
bool parse_setup(Arguments& arguments, int32_t argc, const char8_t** argv);

/** Run ast-tool setup. Returns true when all requested integrations succeeded. */
bool run_setup(const ArgSetup& args);

/** Returns the absolute path to the current executable. Empty on failure. */
std::filesystem::path get_executable_path();

/**
 * Spawn exe + args as a fully detached background process.
 * Returns immediately; the child runs independently.
 * On POSIX uses fork+setsid+execv.  On Windows uses DETACHED_PROCESS.
 */
bool spawn_background(const std::filesystem::path& exe,
                      const std::vector<std::string>& args);

} // namespace ast
#endif // INC_AST_SETUP_H_
