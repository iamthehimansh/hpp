// args.c rewritten in H++
// CLI argument parsing

import std/{sys, mem, str, io};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn str_eq(a: long, b: long) -> int;
fn arg_str(argv: long, index: int) -> long;

// CompilerArgs (32 bytes): input@0, output@8, target@16,
//   asm_only@24, lib_mode@25, dump_tokens@26, dump_ast@27
const CA_INPUT   = 0;
const CA_OUTPUT  = 8;
const CA_TARGET  = 16;
const CA_ASM     = 24;
const CA_LIB     = 25;
const CA_DTOK    = 26;
const CA_DAST    = 27;
const CA_SIZE    = 32;

fn args_print_usage() {
    // Write to stderr (fd 2)
    long msg = "Usage: hpp <source.hpp> [options]\n\nOptions:\n  -o <output>      Output file name\n  -t <target>      Target: linux (default)\n  -S               Output assembly only (no linking)\n  --lib            Compile as library (no _start)\n  --dump-tokens    Print token stream\n  --dump-ast       Print AST\n  --help           Show this help\n";
    long len = str_len(msg);
    sys_write(2, msg, len);
}

fn args_parse(argc: int, argv: long, out: long) -> int {
    // Zero out struct
    mem_zero(out, CA_SIZE);
    // Set default target
    mem_write64(out, CA_TARGET, "linux");

    int i = 1;
    while (i < argc) {
        long arg = arg_str(argv, i);

        if (str_eq(arg, "--help") != 0) {
            args_print_usage();
            return 0;
        } else {
            if (str_eq(arg, "-o") != 0) {
                i++;
                if (i >= argc) {
                    eprint("error: -o requires an argument\n");
                    return 0;
                }
                mem_write64(out, CA_OUTPUT, arg_str(argv, i));
            } else {
                if (str_eq(arg, "-t") != 0) {
                    i++;
                    if (i >= argc) {
                        eprint("error: -t requires an argument\n");
                        return 0;
                    }
                    long tgt = arg_str(argv, i);
                    if (str_eq(tgt, "linux") == 0) {
                        eprint("error: unsupported target (only 'linux')\n");
                        return 0;
                    }
                    mem_write64(out, CA_TARGET, tgt);
                } else {
                    if (str_eq(arg, "-S") != 0) {
                        mem_write8(out, CA_ASM, 1);
                    } else {
                        if (str_eq(arg, "--lib") != 0) {
                            mem_write8(out, CA_LIB, 1);
                        } else {
                            if (str_eq(arg, "-c") != 0) {
                                mem_write8(out, CA_LIB, 1);
                            } else {
                                if (str_eq(arg, "--dump-tokens") != 0) {
                                    mem_write8(out, CA_DTOK, 1);
                                } else {
                                    if (str_eq(arg, "--dump-ast") != 0) {
                                        mem_write8(out, CA_DAST, 1);
                                    } else {
                                        // Check if starts with '-'
                                        int first = mem_read8(arg, 0);
                                        if (first == 45) {
                                            eprint("error: unknown option\n");
                                            return 0;
                                        } else {
                                            if (mem_read64(out, CA_INPUT) != 0) {
                                                eprint("error: multiple input files\n");
                                                return 0;
                                            }
                                            mem_write64(out, CA_INPUT, arg);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        i++;
    }

    if (mem_read64(out, CA_INPUT) == 0) {
        args_print_usage();
        return 0;
    }

    return 1;
}

fn eprint(msg: long) {
    long len = str_len(msg);
    sys_write(2, msg, len);
}
