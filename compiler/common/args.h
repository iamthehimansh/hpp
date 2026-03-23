#ifndef HPP_ARGS_H
#define HPP_ARGS_H

#include <stdbool.h>

typedef struct {
    const char *input_file;
    const char *output_file;
    const char *target;
    bool asm_only;
    bool lib_mode;
    bool dump_tokens;
    bool dump_ast;
} CompilerArgs;

bool args_parse(int argc, char **argv, CompilerArgs *out);
void args_print_usage(void);

#endif
