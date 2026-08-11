#include "ast-tool.h"
#include <cstdio>

int main(int argc, char** argv)
{
    ast::initialize();

    ast::Arguments arguments;
    if(!ast::parse(arguments, argc, const_cast<const char**>(argv))) {
        fprintf(stderr, "error: unknown command or missing required arguments\n");
        fprintf(stderr, "Run 'ast-tool help' to see available commands.\n");
        return 1;
    }
    return ast::dispatch(arguments) ? 0 : 1;
}

