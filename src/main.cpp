#include "ast-tool.h"
#include <cstdio>

int main(int argc, char** argv)
{
    ast::initialize();

    ast::Arguments arguments;
    if(!ast::parse(arguments, argc, const_cast<const char**>(argv))) {
        fprintf(stderr, "usage: %s <dump|symbols|outline|find> [options] <input>\n", 0 < argc ? argv[0] : "ast-tool");
        return 1;
    }
    return ast::dispatch(arguments) ? 0 : 1;
}

