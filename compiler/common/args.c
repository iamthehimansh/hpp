#include "common/args.h"
#include <stdio.h>
#include <string.h>

void args_print_usage(void) {
    fprintf(stderr,
        "Usage: hpp <source.hpp> [options]\n"
        "\n"
        "Options:\n"
        "  -o <output>      Output file name\n"
        "  -t <target>      Target: linux (default)\n"
        "  -S               Output assembly only (no linking)\n"
        "  --dump-tokens    Print token stream\n"
        "  --dump-ast       Print AST\n"
        "  --help           Show this help\n"
    );
}

bool args_parse(int argc, char **argv, CompilerArgs *out) {
    memset(out, 0, sizeof(*out));
    out->target = "linux";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            args_print_usage();
            return false;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -o requires an argument\n");
                return false;
            }
            out->output_file = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -t requires an argument\n");
                return false;
            }
            out->target = argv[++i];
            if (strcmp(out->target, "linux") != 0) {
                fprintf(stderr, "error: unsupported target '%s' (only 'linux' is supported)\n", out->target);
                return false;
            }
        } else if (strcmp(argv[i], "-S") == 0) {
            out->asm_only = true;
        } else if (strcmp(argv[i], "--lib") == 0 || strcmp(argv[i], "-c") == 0) {
            out->lib_mode = true;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            out->dump_tokens = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            out->dump_ast = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            return false;
        } else {
            if (out->input_file != NULL) {
                fprintf(stderr, "error: multiple input files not supported\n");
                return false;
            }
            out->input_file = argv[i];
        }
    }

    if (out->input_file == NULL) {
        args_print_usage();
        return false;
    }

    return true;
}
