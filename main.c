#define __BYTES_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION
#include "types.h"
#include "compiler.h"
#include "parser.h"
#include "da.h"
#include "lexer.h"
#include <stddef.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    struct compiler_t comp = {0};
    init_compiler(&comp);
    comp_set_output(&comp, stdout); //FIXME: compiling to stdout because all the big languages do so
    comp_file(&comp, "main.ar");
    return 0;
}
