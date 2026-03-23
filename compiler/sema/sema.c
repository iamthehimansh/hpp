#include "sema/sema.h"
#include "common/error.h"
#include "common/strtab.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/*  Forward declarations                                               */
/* ------------------------------------------------------------------ */

static void     sema_collect_defs(Sema *sema, AstNode *program);
static void     sema_decl(Sema *sema, AstNode *node);
static void     sema_type_def(Sema *sema, AstNode *node);
static void     sema_func_def(Sema *sema, AstNode *node);
static void     sema_opp_def(Sema *sema, AstNode *node);
static void     sema_block(Sema *sema, AstNode *node);
static void     sema_stmt(Sema *sema, AstNode *node);
static void     sema_var_decl(Sema *sema, AstNode *node);
static void     sema_return_stmt(Sema *sema, AstNode *node);
static void     sema_if_stmt(Sema *sema, AstNode *node);
static void     sema_while_stmt(Sema *sema, AstNode *node);
static void     sema_for_stmt(Sema *sema, AstNode *node);
static HppType *sema_expr(Sema *sema, AstNode *node);
static HppType *sema_binary(Sema *sema, AstNode *node);
static HppType *sema_unary(Sema *sema, AstNode *node);
static HppType *sema_call(Sema *sema, AstNode *node);
static HppType *sema_assign(Sema *sema, AstNode *node);
static HppType *sema_cast(Sema *sema, AstNode *node);
static HppType *sema_ident(Sema *sema, AstNode *node);
static HppType *sema_int_lit(Sema *sema, AstNode *node);
static HppType *sema_bool_lit(Sema *sema, AstNode *node);
static HppType *sema_string_lit(Sema *sema, AstNode *node);
static HppType *sema_resolve_type(Sema *sema, AstNode *type_node);
static void     sema_check_condition(Sema *sema, AstNode *expr);
static bool     sema_literal_fits(uint64_t value, uint32_t bits);
static bool     sema_coerce_literal(Sema *sema, AstNode *node, HppType *target);

static void sema_error(Sema *sema, SourceLoc loc, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* ------------------------------------------------------------------ */
/*  Error helper                                                       */
/* ------------------------------------------------------------------ */

static void sema_error(Sema *sema, SourceLoc loc, const char *fmt, ...)
{
    sema->had_error = true;
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    error_report(loc, ERR_SEMA, "%s", buf);
}

/* ------------------------------------------------------------------ */
/*  std_ll built-in functions                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    int param_count;
    uint32_t param_widths[6];
    uint32_t return_width; /* 0 = void */
} StdLLFunc;

static const StdLLFunc std_ll_funcs[] = {
    /* ---- FILE I/O ---- */
    {"sys_read",           3, {32,64,64,0,0,0},           64},
    {"sys_write",          3, {32,64,64,0,0,0},           64},
    {"sys_open",           3, {64,32,32,0,0,0},           32},
    {"sys_close",          1, {32,0,0,0,0,0},             32},
    {"sys_stat",           2, {64,64,0,0,0,0},            32},
    {"sys_fstat",          2, {32,64,0,0,0,0},            32},
    {"sys_lstat",          2, {64,64,0,0,0,0},            32},
    {"sys_lseek",          3, {32,64,32,0,0,0},           64},
    {"sys_pread64",        4, {32,64,64,64,0,0},          64},
    {"sys_pwrite64",       4, {32,64,64,64,0,0},          64},
    {"sys_access",         2, {64,32,0,0,0,0},            32},
    {"sys_pipe",           1, {64,0,0,0,0,0},             32},
    {"sys_dup",            1, {32,0,0,0,0,0},             32},
    {"sys_dup2",           2, {32,32,0,0,0,0},            32},
    {"sys_fcntl",          3, {32,32,64,0,0,0},           32},
    {"sys_fsync",          1, {32,0,0,0,0,0},             32},
    {"sys_truncate",       2, {64,64,0,0,0,0},            32},
    {"sys_ftruncate",      2, {32,64,0,0,0,0},            32},
    {"sys_rename",         2, {64,64,0,0,0,0},            32},
    {"sys_unlink",         1, {64,0,0,0,0,0},             32},
    {"sys_readlink",       3, {64,64,64,0,0,0},           64},
    {"sys_chmod",          2, {64,32,0,0,0,0},            32},
    {"sys_fchmod",         2, {32,32,0,0,0,0},            32},
    {"sys_chown",          3, {64,32,32,0,0,0},           32},
    /* ---- MEMORY ---- */
    {"sys_mmap",           6, {64,64,32,32,32,64},        64},
    {"sys_mprotect",       3, {64,64,32,0,0,0},           32},
    {"sys_munmap",         2, {64,64,0,0,0,0},            32},
    {"sys_brk",            1, {64,0,0,0,0,0},             64},
    {"sys_mremap",         5, {64,64,64,32,64},           64},
    /* ---- PROCESS ---- */
    {"sys_getpid",         0, {0,0,0,0,0,0},              32},
    {"sys_fork",           0, {0,0,0,0,0,0},              32},
    {"sys_execve",         3, {64,64,64,0,0,0},           32},
    {"sys_exit",           1, {32,0,0,0,0,0},             0},
    {"sys_wait4",          4, {32,64,32,64,0,0},          32},
    {"sys_kill",           2, {32,32,0,0,0,0},            32},
    {"sys_getuid",         0, {0,0,0,0,0,0},              32},
    {"sys_getgid",         0, {0,0,0,0,0,0},              32},
    {"sys_setuid",         1, {32,0,0,0,0,0},             32},
    {"sys_setgid",         1, {32,0,0,0,0,0},             32},
    {"sys_geteuid",        0, {0,0,0,0,0,0},              32},
    {"sys_getegid",        0, {0,0,0,0,0,0},              32},
    {"sys_getppid",        0, {0,0,0,0,0,0},              32},
    {"sys_setsid",         0, {0,0,0,0,0,0},              32},
    {"sys_exit_group",     1, {32,0,0,0,0,0},             0},
    /* ---- DIRECTORY ---- */
    {"sys_getcwd",         2, {64,64,0,0,0,0},            64},
    {"sys_chdir",          1, {64,0,0,0,0,0},             32},
    {"sys_mkdir",          2, {64,32,0,0,0,0},            32},
    {"sys_rmdir",          1, {64,0,0,0,0,0},             32},
    {"sys_getdents64",     3, {32,64,64,0,0,0},           32},
    /* ---- NETWORK ---- */
    {"sys_socket",         3, {32,32,32,0,0,0},           32},
    {"sys_bind",           3, {32,64,32,0,0,0},           32},
    {"sys_listen",         2, {32,32,0,0,0,0},            32},
    {"sys_accept",         3, {32,64,64,0,0,0},           32},
    {"sys_connect",        3, {32,64,32,0,0,0},           32},
    {"sys_sendto",         6, {32,64,64,32,64,32},        64},
    {"sys_recvfrom",       6, {32,64,64,32,64,64},        64},
    {"sys_shutdown",       2, {32,32,0,0,0,0},            32},
    {"sys_setsockopt",     5, {32,32,32,64,32},           32},
    {"sys_getsockopt",     5, {32,32,32,64,64},           32},
    /* ---- TIME ---- */
    {"sys_nanosleep",      2, {64,64,0,0,0,0},            32},
    {"sys_gettimeofday",   2, {64,64,0,0,0,0},            32},
    {"sys_time",           1, {64,0,0,0,0,0},             64},
    {"sys_clock_gettime",  2, {32,64,0,0,0,0},            32},
    /* ---- SIGNAL ---- */
    {"sys_rt_sigaction",   4, {32,64,64,64,0,0},          32},
    {"sys_rt_sigprocmask", 4, {32,64,64,64,0,0},          32},
    {"sys_pause",          0, {0,0,0,0,0,0},              32},
    {"sys_alarm",          1, {32,0,0,0,0,0},             32},
    /* ---- MISC ---- */
    {"sys_poll",           3, {64,32,32,0,0,0},           32},
    {"sys_ioctl",          3, {32,64,64,0,0,0},           32},
    {"sys_select",         5, {32,64,64,64,64},           32},
    {"sys_uname",          1, {64,0,0,0,0,0},             32},
    {"sys_getrusage",      2, {32,64,0,0,0,0},            32},
    {"sys_sysinfo",        1, {64,0,0,0,0,0},             32},
    {"sys_getrandom",      3, {64,64,32,0,0,0},           64},

    /* ============================================ */
    /* std_util — high-level utility functions       */
    /* ============================================ */

    /* ---- OUTPUT ---- */
    {"print_char",         1, {32,0,0,0,0,0},             0},
    {"print_newline",      0, {0,0,0,0,0,0},              0},
    {"print_str",          2, {64,64,0,0,0,0},            0},
    {"puts",               1, {64,0,0,0,0,0},             0},
    {"println",            1, {64,0,0,0,0,0},             0},
    {"eputs",              1, {64,0,0,0,0,0},             0},
    {"print_int",          1, {32,0,0,0,0,0},             0},
    {"print_long",         1, {64,0,0,0,0,0},             0},
    {"print_signed",       1, {32,0,0,0,0,0},             0},
    {"print_hex",          1, {64,0,0,0,0,0},             0},
    {"eprint_str",         2, {64,64,0,0,0,0},            0},
    /* ---- INPUT ---- */
    {"read_char",          0, {0,0,0,0,0,0},              32},
    {"read_stdin",         2, {64,64,0,0,0,0},            64},
    /* ---- MEMORY OPS ---- */
    {"mem_copy",           3, {64,64,64,0,0,0},           0},
    {"mem_set",            3, {64,32,64,0,0,0},           0},
    {"mem_cmp",            3, {64,64,64,0,0,0},           32},
    {"mem_zero",           2, {64,64,0,0,0,0},            0},
    {"mem_read8",          2, {64,32,0,0,0,0},            32},
    {"mem_read16",         2, {64,32,0,0,0,0},            32},
    {"mem_read32",         2, {64,32,0,0,0,0},            32},
    {"mem_read64",         2, {64,32,0,0,0,0},            64},
    {"mem_write8",         3, {64,32,32,0,0,0},           0},
    {"mem_write16",        3, {64,32,32,0,0,0},           0},
    {"mem_write32",        3, {64,32,32,0,0,0},           0},
    {"mem_write64",        3, {64,32,64,0,0,0},           0},
    /* ---- STRING OPS ---- */
    {"str_len",            1, {64,0,0,0,0,0},             64},
    {"str_eq",             2, {64,64,0,0,0,0},            32},
    {"str_copy",           2, {64,64,0,0,0,0},            64},
    {"str_cat",            2, {64,64,0,0,0,0},            64},
    {"str_chr",            2, {64,32,0,0,0,0},            64},
    /* ---- NUMBER CONVERSION ---- */
    {"itoa",               2, {32,64,0,0,0,0},            64},
    {"atoi",               1, {64,0,0,0,0,0},             32},
    /* ---- ALLOCATION ---- */
    {"alloc",              1, {64,0,0,0,0,0},             64},
    {"free",               2, {64,64,0,0,0,0},            32},
    {"realloc",            3, {64,64,64,0,0,0},           64},
    /* ---- FILE HELPERS ---- */
    {"file_open_read",     1, {64,0,0,0,0,0},             32},
    {"file_open_write",    1, {64,0,0,0,0,0},             32},
    {"file_open_append",   1, {64,0,0,0,0,0},             32},
    {"file_read",          3, {32,64,64,0,0,0},           64},
    {"file_write",         3, {32,64,64,0,0,0},           64},
    {"file_close",         1, {32,0,0,0,0,0},             32},
    {"file_size",          1, {32,0,0,0,0,0},             64},
    /* ---- PROCESS HELPERS ---- */
    {"sleep_ms",           1, {32,0,0,0,0,0},             0},
    {"sleep_sec",          1, {32,0,0,0,0,0},             0},
    {"get_time_sec",       0, {0,0,0,0,0,0},              64},
    /* ---- RANDOM ---- */
    {"random_bytes",       2, {64,64,0,0,0,0},            64},
    {"random_int",         0, {0,0,0,0,0,0},              32},
    /* ---- MISC ---- */
    {"exit",               1, {32,0,0,0,0,0},             0},
    {"abort",              0, {0,0,0,0,0,0},              0},
    {"arg_str",            2, {64,32,0,0,0,0},            64},
    /* ---- STRING BUILDER ---- */
    {"sb_new",             0, {0,0,0,0,0,0},              64},
    {"sb_free",            1, {64,0,0,0,0,0},             0},
    {"sb_len",             1, {64,0,0,0,0,0},             32},
    {"sb_data",            1, {64,0,0,0,0,0},             64},
    {"sb_append",          2, {64,64,0,0,0,0},            0},
    {"sb_append_str",      3, {64,64,32,0,0,0},           0},
    {"sb_append_char",     2, {64,32,0,0,0,0},            0},
    {"sb_append_int",      2, {64,32,0,0,0,0},            0},
    {"sb_append_long",     2, {64,64,0,0,0,0},            0},
    {"sb_append_hex",      2, {64,64,0,0,0,0},            0},
    {"sb_to_str",          1, {64,0,0,0,0,0},             64},
    {"sb_clear",           1, {64,0,0,0,0,0},             0},
};

#define STD_LL_COUNT ((int)(sizeof(std_ll_funcs) / sizeof(std_ll_funcs[0])))

/* Backward-compatible registration for programs without imports */
void register_std_ll_compat(Sema *sema)
{
    SourceLoc builtin_loc = { .filename = "<builtin>", .line = 0, .col = 0 };

    for (int i = 0; i < STD_LL_COUNT; i++) {
        const StdLLFunc *f = &std_ll_funcs[i];

        /* Build parameter types */
        HppType **param_types = NULL;
        if (f->param_count > 0) {
            param_types = arena_alloc(sema->arena,
                              (size_t)f->param_count * sizeof(HppType *));
            for (int j = 0; j < f->param_count; j++) {
                if (f->param_widths[j] == 1) {
                    param_types[j] = &HPP_TYPE_BIT;
                } else {
                    param_types[j] = type_make_bitn(sema->arena,
                                                    f->param_widths[j]);
                }
            }
        }

        /* Return type */
        HppType *ret;
        if (f->return_width == 0) {
            ret = &HPP_TYPE_VOID;
        } else if (f->return_width == 1) {
            ret = &HPP_TYPE_BIT;
        } else {
            ret = type_make_bitn(sema->arena, f->return_width);
        }

        const char *iname = strtab_intern(sema->strtab, f->name, strlen(f->name));
        Symbol *sym = symtab_define(sema->symbols, iname,
                                    SYM_FUNC, ret, builtin_loc);
        if (sym != NULL) {
            sym->is_external           = true;
            sym->func_info.param_types = param_types;
            sym->func_info.param_count = f->param_count;
            sym->func_info.return_type = ret;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Register module declarations                                       */
/* ------------------------------------------------------------------ */

void sema_register_module(Sema *sema, AstNode *module_program)
{
    if (!module_program || module_program->kind != NODE_PROGRAM) return;

    for (AstNodeList *n = module_program->as.program.decls; n; n = n->next) {
        AstNode *node = n->node;
        if (!node) continue;

        if (node->kind == NODE_TYPE_DEF) {
            /* Register type definition */
            const char *name = node->as.type_def.name;
            uint32_t width = node->as.type_def.bit_width;
            if (!type_table_lookup(sema->types, name)) {
                type_table_define(sema->types, name, width);
            }
        }
        else if (node->kind == NODE_FUNC_DEF) {
            /* Register function declaration (no body expected in .hdef) */
            const char *name = node->as.func_def.name;
            const char *iname = strtab_intern(sema->strtab, name, strlen(name));

            /* Skip if already registered */
            if (symtab_lookup(sema->symbols, iname) != NULL) continue;

            /* Resolve return type */
            HppType *ret = &HPP_TYPE_VOID;
            if (node->as.func_def.return_type) {
                ret = sema_resolve_type(sema, node->as.func_def.return_type);
                if (!ret) ret = &HPP_TYPE_VOID;
            }

            /* Resolve parameter types */
            int pc = node->as.func_def.params.count;
            HppType **param_types = NULL;
            if (pc > 0) {
                param_types = arena_alloc(sema->arena, (size_t)pc * sizeof(HppType *));
                for (int i = 0; i < pc; i++) {
                    Param *p = &node->as.func_def.params.params[i];
                    HppType *pt = sema_resolve_type(sema, p->type_node);
                    param_types[i] = pt ? pt : type_make_bitn(sema->arena, 64);
                }
            }

            Symbol *sym = symtab_define(sema->symbols, iname,
                                        SYM_FUNC, ret, node->loc);
            if (sym) {
                sym->is_external = true;
                sym->func_info.param_types = param_types;
                sym->func_info.param_count = pc;
                sym->func_info.return_type = ret;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Init / analyze                                                     */
/* ------------------------------------------------------------------ */

void sema_init(Sema *sema, Arena *arena, StrTab *strtab, const char *filename)
{
    memset(sema, 0, sizeof(Sema));
    sema->arena    = arena;
    sema->strtab   = strtab;
    sema->filename = filename;

    sema->types = type_table_create(arena);
    type_table_init_builtins(sema->types);

    sema->symbols = symtab_create(arena);
    sema->opps    = opp_table_create(arena);

    /* std_ll functions are no longer auto-registered.
       They are loaded via `import` directives.
       Keep register_std_ll as fallback for programs with no imports. */
}

bool sema_analyze(Sema *sema, AstNode *program)
{
    if (program == NULL || program->kind != NODE_PROGRAM) {
        sema_error(sema, program ? program->loc : (SourceLoc){NULL,0,0},
                   "expected program node");
        return false;
    }

    /* Pass 1: collect top-level definitions */
    sema_collect_defs(sema, program);

    /* Pass 2: full analysis */
    for (AstNodeList *n = program->as.program.decls; n != NULL; n = n->next) {
        sema_decl(sema, n->node);
    }

    /* Check that main exists (skip for library modules) */
    if (!sema->is_library) {
        const char *main_str = strtab_intern(sema->strtab, "main", 4);
        Symbol *main_sym = symtab_lookup(sema->symbols, main_str);
        if (main_sym == NULL || main_sym->kind != SYM_FUNC) {
            sema_error(sema, program->loc, "program has no 'main' function");
        }
    }

    return !sema->had_error;
}

/* ------------------------------------------------------------------ */
/*  Pass 1: collect forward declarations                               */
/* ------------------------------------------------------------------ */

static void sema_collect_defs(Sema *sema, AstNode *program)
{
    for (AstNodeList *n = program->as.program.decls; n != NULL; n = n->next) {
        AstNode *node = n->node;
        if (node == NULL) {
            continue;
        }

        switch (node->kind) {
        case NODE_TYPE_DEF:
            sema_type_def(sema, node);
            break;

        case NODE_FUNC_DEF: {
            /* Register signature only */
            const char *name = node->as.func_def.name;

            /* Resolve return type */
            HppType *ret = &HPP_TYPE_VOID;
            if (node->as.func_def.return_type != NULL) {
                ret = sema_resolve_type(sema, node->as.func_def.return_type);
                if (ret == NULL) {
                    ret = &HPP_TYPE_VOID;
                }
            }

            /* Resolve parameter types */
            int pc = node->as.func_def.params.count;
            HppType **ptypes = NULL;
            if (pc > 0) {
                ptypes = arena_alloc(sema->arena, (size_t)pc * sizeof(HppType *));
                for (int i = 0; i < pc; i++) {
                    Param *p = &node->as.func_def.params.params[i];
                    ptypes[i] = sema_resolve_type(sema, p->type_node);
                    if (ptypes[i] == NULL) {
                        sema_error(sema, p->loc,
                                   "unresolved type for parameter '%s'",
                                   p->name);
                        ptypes[i] = type_make_bitn(sema->arena, 32);
                    }
                }
            }

            /* Check if already declared (e.g. from .hdef import) */
            Symbol *existing = symtab_lookup(sema->symbols, name);
            if (existing && existing->kind == SYM_FUNC && existing->is_external
                && node->as.func_def.body != NULL) {
                /* User is providing body for an imported declaration — override */
                existing->func_info.param_types = ptypes;
                existing->func_info.param_count = pc;
                existing->func_info.return_type = ret;
                existing->is_external = false;
                existing->loc = node->loc;
            } else {
                Symbol *sym = symtab_define(sema->symbols, name,
                                            SYM_FUNC, ret, node->loc);
                if (sym == NULL) {
                    /* Duplicate of non-external — real error, unless it's a
                       bodyless declaration from .hdef (which we ignore) */
                    if (node->as.func_def.body != NULL) {
                        sema_error(sema, node->loc,
                                   "duplicate function definition '%s'", name);
                    }
                } else {
                    sym->func_info.param_types = ptypes;
                    sym->func_info.param_count = pc;
                    sym->func_info.return_type = ret;
                    sym->is_external = (node->as.func_def.body == NULL);
                }
            }
            break;
        }

        case NODE_OPP_DEF:
            sema_opp_def(sema, node);
            break;

        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Type definition                                                    */
/* ------------------------------------------------------------------ */

static void sema_type_def(Sema *sema, AstNode *node)
{
    const char *name = node->as.type_def.name;
    uint32_t width   = node->as.type_def.bit_width;

    HppType *existing = type_table_lookup(sema->types, name);
    if (existing) {
        /* Allow re-definition if same width (from imported .hdef) */
        if (existing->bit_width != width) {
            sema_error(sema, node->loc,
                       "conflicting type definition '%s' (was bit[%u], now bit[%u])",
                       name, existing->bit_width, width);
        }
        return;
    }
    HppType *t = type_table_define(sema->types, name, width);
    (void)t;
}

/* ------------------------------------------------------------------ */
/*  Opp definition                                                     */
/* ------------------------------------------------------------------ */

static void sema_opp_def(Sema *sema, AstNode *node)
{
    OppEntry entry;
    memset(&entry, 0, sizeof(entry));

    entry.is_unary = node->as.opp_def.is_unary;

    if (entry.is_unary) {
        entry.unop = node->as.opp_def.unary_op;
        entry.left_type = sema_resolve_type(sema, node->as.opp_def.left_type);
        if (entry.left_type == NULL) {
            sema_error(sema, node->loc,
                       "unresolved operand type in opp definition");
            return;
        }
        entry.right_type = NULL;
    } else {
        entry.binop = node->as.opp_def.op;
        entry.left_type = sema_resolve_type(sema, node->as.opp_def.left_type);
        entry.right_type = sema_resolve_type(sema, node->as.opp_def.right_type);
        if (entry.left_type == NULL || entry.right_type == NULL) {
            sema_error(sema, node->loc,
                       "unresolved operand type in opp definition");
            return;
        }
    }

    entry.result_type = sema_resolve_type(sema, node->as.opp_def.result_type);
    if (entry.result_type == NULL) {
        sema_error(sema, node->loc,
                   "unresolved result type in opp definition");
        return;
    }

    entry.asm_blocks = node->as.opp_def.asm_blocks;
    opp_table_register(sema->opps, &entry);
}

/* ------------------------------------------------------------------ */
/*  Pass 2 dispatcher                                                  */
/* ------------------------------------------------------------------ */

static void sema_decl(Sema *sema, AstNode *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->kind) {
    case NODE_TYPE_DEF:
        /* Already handled in pass 1 */
        break;
    case NODE_FUNC_DEF:
        sema_func_def(sema, node);
        break;
    case NODE_OPP_DEF:
        /* Already handled in pass 1 */
        break;
    case NODE_IMPORT:
    case NODE_LINK:
        /* Handled before sema in main.c */
        break;
    default:
        sema_error(sema, node->loc,
                   "unexpected top-level node: %s",
                   ast_node_kind_str(node->kind));
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Function definition (pass 2 — full body analysis)                  */
/* ------------------------------------------------------------------ */

static void sema_func_def(Sema *sema, AstNode *node)
{
    /* Skip declaration-only functions (no body, from .hdef files) */
    if (node->as.func_def.body == NULL) return;

    Symbol *func_sym = symtab_lookup(sema->symbols, node->as.func_def.name);
    if (func_sym == NULL) {
        /* Should not happen — registered in pass 1 */
        return;
    }

    HppType *prev_ret = sema->current_func_return_type;
    sema->current_func_return_type = func_sym->func_info.return_type;

    symtab_enter_scope(sema->symbols);

    /* Add parameters */
    int pc = node->as.func_def.params.count;
    for (int i = 0; i < pc; i++) {
        Param *p = &node->as.func_def.params.params[i];
        Symbol *psym = symtab_define(sema->symbols, p->name,
                                     SYM_PARAM,
                                     func_sym->func_info.param_types[i],
                                     p->loc);
        if (psym == NULL) {
            sema_error(sema, p->loc,
                       "duplicate parameter name '%s'", p->name);
        } else {
            psym->is_mutable = false;
        }
    }

    /* Analyze body */
    if (node->as.func_def.body != NULL) {
        sema_block(sema, node->as.func_def.body);
    }

    symtab_exit_scope(sema->symbols);
    sema->current_func_return_type = prev_ret;
}

/* ------------------------------------------------------------------ */
/*  Block                                                              */
/* ------------------------------------------------------------------ */

static void sema_block(Sema *sema, AstNode *node)
{
    if (node == NULL || node->kind != NODE_BLOCK) {
        return;
    }

    symtab_enter_scope(sema->symbols);

    for (AstNodeList *n = node->as.block.stmts; n != NULL; n = n->next) {
        sema_stmt(sema, n->node);
    }

    symtab_exit_scope(sema->symbols);
}

/* ------------------------------------------------------------------ */
/*  Statement dispatcher                                               */
/* ------------------------------------------------------------------ */

static void sema_stmt(Sema *sema, AstNode *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->kind) {
    case NODE_VAR_DECL:      sema_var_decl(sema, node);    break;
    case NODE_RETURN_STMT:   sema_return_stmt(sema, node); break;
    case NODE_IF_STMT:       sema_if_stmt(sema, node);     break;
    case NODE_WHILE_STMT:    sema_while_stmt(sema, node);  break;
    case NODE_FOR_STMT:      sema_for_stmt(sema, node);    break;
    case NODE_BLOCK:         sema_block(sema, node);        break;
    case NODE_ASM_BLOCK:     /* nothing to check */         break;

    case NODE_SWITCH_STMT: {
        sema_expr(sema, node->as.switch_stmt.expr);
        sema->loop_depth++;  /* allow break inside switch */
        for (int i = 0; i < node->as.switch_stmt.case_count; i++) {
            sema_block(sema, node->as.switch_stmt.case_bodies[i]);
        }
        if (node->as.switch_stmt.default_body) {
            sema_block(sema, node->as.switch_stmt.default_body);
        }
        sema->loop_depth--;
        break;
    }

    case NODE_BREAK_STMT:
    case NODE_CONTINUE_STMT:
        if (sema->loop_depth == 0) {
            sema_error(sema, node->loc,
                       "'%s' outside of loop",
                       node->kind == NODE_BREAK_STMT ? "break" : "continue");
        }
        break;

    case NODE_EXPR_STMT:
        if (node->as.expr_stmt.expr != NULL) {
            HppType *t = sema_expr(sema, node->as.expr_stmt.expr);
            /* If int literal is still unresolved, default to bit[32] */
            if (t == NULL && node->as.expr_stmt.expr->kind == NODE_INT_LIT) {
                node->as.expr_stmt.expr->resolved_type =
                    type_make_bitn(sema->arena, 32);
            }
        }
        break;

    default:
        sema_error(sema, node->loc,
                   "unexpected statement: %s",
                   ast_node_kind_str(node->kind));
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Variable declaration                                               */
/* ------------------------------------------------------------------ */

static void sema_var_decl(Sema *sema, AstNode *node)
{
    const char *name = node->as.var_decl.name;
    DeclKind dk      = node->as.var_decl.decl_kind;

    switch (dk) {
    case DECL_CONST: {
        if (node->as.var_decl.init_expr == NULL) {
            sema_error(sema, node->loc,
                       "const '%s' requires an initializer", name);
            return;
        }
        HppType *init_type = sema_expr(sema, node->as.var_decl.init_expr);
        /* Default unresolved literals to bit[32] */
        if (init_type == NULL) {
            init_type = type_make_bitn(sema->arena, 32);
            node->as.var_decl.init_expr->resolved_type = init_type;
        }
        Symbol *sym = symtab_define(sema->symbols, name, SYM_CONST,
                                    init_type, node->loc);
        if (sym == NULL) {
            sema_error(sema, node->loc,
                       "duplicate definition '%s' in current scope", name);
        } else {
            sym->is_mutable = false;
        }
        break;
    }

    case DECL_LET: {
        if (node->as.var_decl.init_expr == NULL) {
            sema_error(sema, node->loc,
                       "let '%s' requires an initializer", name);
            return;
        }
        HppType *init_type = sema_expr(sema, node->as.var_decl.init_expr);
        if (init_type == NULL) {
            init_type = type_make_bitn(sema->arena, 32);
            node->as.var_decl.init_expr->resolved_type = init_type;
        }
        Symbol *sym = symtab_define(sema->symbols, name, SYM_VAR,
                                    init_type, node->loc);
        if (sym == NULL) {
            sema_error(sema, node->loc,
                       "duplicate definition '%s' in current scope", name);
        } else {
            sym->is_mutable = true;
        }
        break;
    }

    case DECL_TYPED: {
        HppType *declared = sema_resolve_type(sema, node->as.var_decl.type_node);
        if (declared == NULL) {
            sema_error(sema, node->loc,
                       "unresolved type in declaration of '%s'", name);
            return;
        }
        if (node->as.var_decl.init_expr != NULL) {
            HppType *init_type = sema_expr(sema, node->as.var_decl.init_expr);
            /* Coerce literal to declared type */
            if (init_type == NULL) {
                if (!sema_coerce_literal(sema, node->as.var_decl.init_expr,
                                         declared)) {
                    sema_error(sema, node->as.var_decl.init_expr->loc,
                               "initializer value does not fit in type '%s' "
                               "(%u bits)",
                               declared->name ? declared->name : "bit",
                               declared->bit_width);
                }
            } else if (!type_equivalent(init_type, declared)) {
                /* Allow implicit widening (e.g. int → long) */
                if (!type_can_widen(init_type, declared)) {
                    sema_error(sema, node->as.var_decl.init_expr->loc,
                               "type mismatch: initializer is %u bits, "
                               "expected %u bits",
                               init_type->bit_width, declared->bit_width);
                }
            }
        }
        Symbol *sym = symtab_define(sema->symbols, name, SYM_VAR,
                                    declared, node->loc);
        if (sym == NULL) {
            sema_error(sema, node->loc,
                       "duplicate definition '%s' in current scope", name);
        } else {
            sym->is_mutable = true;
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------ */
/*  Return statement                                                   */
/* ------------------------------------------------------------------ */

static void sema_return_stmt(Sema *sema, AstNode *node)
{
    if (sema->current_func_return_type == NULL) {
        sema_error(sema, node->loc, "return outside of function");
        return;
    }

    bool returns_void = (sema->current_func_return_type->bit_width == 0);

    if (node->as.return_stmt.expr == NULL) {
        if (!returns_void) {
            sema_error(sema, node->loc,
                       "non-void function must return a value");
        }
        return;
    }

    if (returns_void) {
        sema_error(sema, node->loc,
                   "void function cannot return a value");
        return;
    }

    HppType *expr_type = sema_expr(sema, node->as.return_stmt.expr);
    if (expr_type == NULL) {
        /* Try to coerce literal */
        if (!sema_coerce_literal(sema, node->as.return_stmt.expr,
                                 sema->current_func_return_type)) {
            sema_error(sema, node->as.return_stmt.expr->loc,
                       "return value does not fit in return type");
        }
        return;
    }

    if (!type_equivalent(expr_type, sema->current_func_return_type)) {
        sema_error(sema, node->as.return_stmt.expr->loc,
                   "return type mismatch: got %u bits, expected %u bits",
                   expr_type->bit_width,
                   sema->current_func_return_type->bit_width);
    }
}

/* ------------------------------------------------------------------ */
/*  If / While / For                                                   */
/* ------------------------------------------------------------------ */

static void sema_if_stmt(Sema *sema, AstNode *node)
{
    if (node->as.if_stmt.condition != NULL) {
        HppType *ct = sema_expr(sema, node->as.if_stmt.condition);
        (void)ct;
        sema_check_condition(sema, node->as.if_stmt.condition);
    }
    if (node->as.if_stmt.then_block != NULL) {
        sema_block(sema, node->as.if_stmt.then_block);
    }
    if (node->as.if_stmt.else_block != NULL) {
        /* else_block can be a block or another if_stmt */
        if (node->as.if_stmt.else_block->kind == NODE_IF_STMT) {
            sema_if_stmt(sema, node->as.if_stmt.else_block);
        } else {
            sema_block(sema, node->as.if_stmt.else_block);
        }
    }
}

static void sema_while_stmt(Sema *sema, AstNode *node)
{
    if (node->as.while_stmt.condition != NULL) {
        HppType *ct = sema_expr(sema, node->as.while_stmt.condition);
        (void)ct;
        sema_check_condition(sema, node->as.while_stmt.condition);
    }
    sema->loop_depth++;
    if (node->as.while_stmt.body != NULL) {
        sema_block(sema, node->as.while_stmt.body);
    }
    sema->loop_depth--;
}

static void sema_for_stmt(Sema *sema, AstNode *node)
{
    symtab_enter_scope(sema->symbols);

    if (node->as.for_stmt.init != NULL) {
        sema_stmt(sema, node->as.for_stmt.init);
    }
    if (node->as.for_stmt.condition != NULL) {
        HppType *ct = sema_expr(sema, node->as.for_stmt.condition);
        (void)ct;
        sema_check_condition(sema, node->as.for_stmt.condition);
    }

    sema->loop_depth++;
    if (node->as.for_stmt.body != NULL) {
        sema_block(sema, node->as.for_stmt.body);
    }
    if (node->as.for_stmt.update != NULL) {
        HppType *ut = sema_expr(sema, node->as.for_stmt.update);
        (void)ut;
    }
    sema->loop_depth--;

    symtab_exit_scope(sema->symbols);
}

/* ------------------------------------------------------------------ */
/*  Expression dispatcher                                              */
/* ------------------------------------------------------------------ */

static HppType *sema_expr(Sema *sema, AstNode *node)
{
    if (node == NULL) {
        return NULL;
    }

    HppType *result = NULL;

    switch (node->kind) {
    case NODE_INT_LIT:      result = sema_int_lit(sema, node);  break;
    case NODE_BOOL_LIT:     result = sema_bool_lit(sema, node); break;
    case NODE_STRING_LIT:   result = sema_string_lit(sema, node); break;
    case NODE_IDENT:        result = sema_ident(sema, node);    break;
    case NODE_BINARY_EXPR:  result = sema_binary(sema, node);   break;
    case NODE_UNARY_EXPR:   result = sema_unary(sema, node);    break;
    case NODE_CALL_EXPR:    result = sema_call(sema, node);     break;
    case NODE_ASSIGN_EXPR:  result = sema_assign(sema, node);   break;
    case NODE_CAST_EXPR:    result = sema_cast(sema, node);     break;
    case NODE_ADDR_OF: {
        /* &var — verify variable exists, return long (pointer) */
        Symbol *sym = symtab_lookup(sema->symbols, node->as.addr_of.name);
        if (!sym) {
            sema_error(sema, node->loc, "undefined variable '%s'",
                       node->as.addr_of.name);
        }
        result = type_make_bitn(sema->arena, 64);
        break;
    }
    case NODE_DEREF: {
        /* *ptr — read from address, returns long */
        HppType *addr_type = sema_expr(sema, node->as.deref.operand);
        (void)addr_type;
        result = type_make_bitn(sema->arena, 64);
        break;
    }
    case NODE_DEREF_ASSIGN: {
        /* *ptr = val — write to address */
        HppType *addr_type = sema_expr(sema, node->as.deref_assign.addr);
        HppType *val_type = sema_expr(sema, node->as.deref_assign.value);
        (void)addr_type;
        (void)val_type;
        result = type_make_bitn(sema->arena, 64);
        break;
    }
    default:
        sema_error(sema, node->loc,
                   "unexpected expression: %s",
                   ast_node_kind_str(node->kind));
        break;
    }

    node->resolved_type = result;
    return result;
}

/* ------------------------------------------------------------------ */
/*  Binary expression                                                  */
/* ------------------------------------------------------------------ */

static bool is_comparison(BinOp op)
{
    return op == BINOP_EQ || op == BINOP_NE ||
           op == BINOP_LT || op == BINOP_GT ||
           op == BINOP_LE || op == BINOP_GE;
}

static bool is_logical(BinOp op)
{
    return op == BINOP_LOGIC_AND || op == BINOP_LOGIC_OR;
}

static bool is_shift(BinOp op)
{
    return op == BINOP_SHL || op == BINOP_SHR;
}

static HppType *sema_binary(Sema *sema, AstNode *node)
{
    BinOp op = node->as.binary.op;
    HppType *left  = sema_expr(sema, node->as.binary.left);
    HppType *right = sema_expr(sema, node->as.binary.right);

    /* Coerce unresolved literals: adopt the other side's type */
    if (left == NULL && right != NULL) {
        sema_coerce_literal(sema, node->as.binary.left, right);
        left = node->as.binary.left->resolved_type;
    }
    if (right == NULL && left != NULL) {
        sema_coerce_literal(sema, node->as.binary.right, left);
        right = node->as.binary.right->resolved_type;
    }
    if (left == NULL && right == NULL) {
        /* Both literals — default to bit[32] */
        HppType *def = type_make_bitn(sema->arena, 32);
        sema_coerce_literal(sema, node->as.binary.left, def);
        sema_coerce_literal(sema, node->as.binary.right, def);
        left  = def;
        right = def;
    }

    if (left == NULL || right == NULL) {
        return NULL;
    }

    /* Check for custom opp overload first */
    OppEntry *opp = opp_table_lookup_binary(sema->opps, left, op, right);
    if (opp != NULL) {
        return opp->result_type;
    }

    /* Logical operators */
    if (is_logical(op)) {
        if (left->bit_width != 1) {
            sema_error(sema, node->as.binary.left->loc,
                       "logical operator requires bit type, got %u bits",
                       left->bit_width);
        }
        if (right->bit_width != 1) {
            sema_error(sema, node->as.binary.right->loc,
                       "logical operator requires bit type, got %u bits",
                       right->bit_width);
        }
        return &HPP_TYPE_BIT;
    }

    /* Comparison operators */
    if (is_comparison(op)) {
        if (!type_equivalent(left, right)) {
            sema_error(sema, node->loc,
                       "comparison operands have different widths: "
                       "%u vs %u bits",
                       left->bit_width, right->bit_width);
        }
        return &HPP_TYPE_BIT;
    }

    /* Shift operators */
    if (is_shift(op)) {
        /* Left determines result type, right can be any integer width */
        if (right->bit_width == 0) {
            sema_error(sema, node->as.binary.right->loc,
                       "shift amount cannot be void");
        }
        return left;
    }

    /* Arithmetic and bitwise: same width required */
    if (!type_equivalent(left, right)) {
        sema_error(sema, node->loc,
                   "binary operands have different widths: %u vs %u bits",
                   left->bit_width, right->bit_width);
    }
    return left;
}

/* ------------------------------------------------------------------ */
/*  Unary expression                                                   */
/* ------------------------------------------------------------------ */

static HppType *sema_unary(Sema *sema, AstNode *node)
{
    UnOp op = node->as.unary.op;
    HppType *operand = sema_expr(sema, node->as.unary.operand);

    if (operand == NULL) {
        /* Unresolved literal */
        if (op == UNOP_NOT) {
            sema_coerce_literal(sema, node->as.unary.operand, &HPP_TYPE_BIT);
            operand = node->as.unary.operand->resolved_type;
        } else {
            HppType *def = type_make_bitn(sema->arena, 32);
            sema_coerce_literal(sema, node->as.unary.operand, def);
            operand = node->as.unary.operand->resolved_type;
        }
    }

    if (operand == NULL) {
        return NULL;
    }

    /* Check for custom opp overload */
    OppEntry *opp = opp_table_lookup_unary(sema->opps, op, operand);
    if (opp != NULL) {
        return opp->result_type;
    }

    switch (op) {
    case UNOP_NOT:
        if (operand->bit_width != 1) {
            sema_error(sema, node->as.unary.operand->loc,
                       "'!' requires bit type, got %u bits",
                       operand->bit_width);
        }
        return &HPP_TYPE_BIT;

    case UNOP_BITNOT:
        return operand;

    case UNOP_NEG:
        return operand;
    }

    return operand; /* unreachable, but satisfies compiler */
}

/* ------------------------------------------------------------------ */
/*  Function call                                                      */
/* ------------------------------------------------------------------ */

static HppType *sema_call(Sema *sema, AstNode *node)
{
    const char *name = node->as.call.func_name;
    Symbol *sym = symtab_lookup(sema->symbols, name);

    if (sym == NULL) {
        sema_error(sema, node->loc,
                   "undefined function '%s'", name);
        return NULL;
    }
    if (sym->kind != SYM_FUNC) {
        /* Allow calling through function pointer (variable holding address) */
        if (sym->kind == SYM_VAR || sym->kind == SYM_PARAM) {
            /* Function pointer call — can't type-check args, just analyze them */
            for (int i = 0; i < node->as.call.arg_count; i++) {
                sema_expr(sema, node->as.call.args[i]);
            }
            /* Return type unknown — assume long (64-bit) */
            return type_make_bitn(sema->arena, 64);
        }
        sema_error(sema, node->loc,
                   "'%s' is not a function", name);
        return NULL;
    }

    int expected = sym->func_info.param_count;
    int got      = node->as.call.arg_count;
    if (got != expected) {
        sema_error(sema, node->loc,
                   "function '%s' expects %d arguments, got %d",
                   name, expected, got);
        return sym->func_info.return_type;
    }

    for (int i = 0; i < got; i++) {
        HppType *arg_type = sema_expr(sema, node->as.call.args[i]);
        HppType *param_type = sym->func_info.param_types[i];

        if (arg_type == NULL) {
            /* Try to coerce literal to parameter type */
            if (!sema_coerce_literal(sema, node->as.call.args[i],
                                     param_type)) {
                sema_error(sema, node->as.call.args[i]->loc,
                           "argument %d of '%s': value does not fit in "
                           "%u bits",
                           i + 1, name, param_type->bit_width);
            }
        } else if (!type_equivalent(arg_type, param_type)) {
            /* Allow implicit widening (e.g. passing int where long expected) */
            if (!type_can_widen(arg_type, param_type)) {
                sema_error(sema, node->as.call.args[i]->loc,
                           "argument %d of '%s': expected %u bits, got %u bits",
                           i + 1, name,
                           param_type->bit_width, arg_type->bit_width);
            }
        }
    }

    return sym->func_info.return_type;
}

/* ------------------------------------------------------------------ */
/*  Assignment                                                         */
/* ------------------------------------------------------------------ */

static HppType *sema_assign(Sema *sema, AstNode *node)
{
    const char *name = node->as.assign.name;
    Symbol *sym = symtab_lookup(sema->symbols, name);

    if (sym == NULL) {
        sema_error(sema, node->loc,
                   "undefined variable '%s'", name);
        return NULL;
    }
    if (!sym->is_mutable) {
        sema_error(sema, node->loc,
                   "cannot assign to immutable '%s'", name);
    }

    HppType *rhs = sema_expr(sema, node->as.assign.value);
    if (rhs == NULL) {
        /* Coerce literal to variable type */
        if (!sema_coerce_literal(sema, node->as.assign.value, sym->type)) {
            sema_error(sema, node->as.assign.value->loc,
                       "assignment value does not fit in type of '%s'", name);
        }
    } else if (!type_equivalent(rhs, sym->type)) {
        /* Allow implicit widening (e.g. int → long) */
        if (!type_can_widen(rhs, sym->type)) {
            sema_error(sema, node->as.assign.value->loc,
                       "type mismatch in assignment to '%s': "
                       "expected %u bits, got %u bits",
                       name, sym->type->bit_width, rhs->bit_width);
        }
    }

    return sym->type;
}

/* ------------------------------------------------------------------ */
/*  Cast expression                                                    */
/* ------------------------------------------------------------------ */

static HppType *sema_cast(Sema *sema, AstNode *node)
{
    HppType *target = sema_resolve_type(sema, node->as.cast.target_type);
    if (target == NULL) {
        sema_error(sema, node->loc, "unresolved cast target type");
        return NULL;
    }

    HppType *src = sema_expr(sema, node->as.cast.expr);
    if (src == NULL) {
        /* Coerce literal to target */
        sema_coerce_literal(sema, node->as.cast.expr, target);
    }

    return target;
}

/* ------------------------------------------------------------------ */
/*  Identifier                                                         */
/* ------------------------------------------------------------------ */

static HppType *sema_ident(Sema *sema, AstNode *node)
{
    const char *name = node->as.ident.name;
    Symbol *sym = symtab_lookup(sema->symbols, name);

    if (sym == NULL) {
        sema_error(sema, node->loc,
                   "undefined identifier '%s'", name);
        return NULL;
    }

    return sym->type;
}

/* ------------------------------------------------------------------ */
/*  Literals                                                           */
/* ------------------------------------------------------------------ */

static HppType *sema_int_lit(Sema *sema, AstNode *node)
{
    (void)sema;
    (void)node;
    /* Return NULL to signal "unresolved" — callers will coerce */
    return NULL;
}

static HppType *sema_bool_lit(Sema *sema, AstNode *node)
{
    (void)sema;
    (void)node;
    return &HPP_TYPE_BIT;
}

static HppType *sema_string_lit(Sema *sema, AstNode *node)
{
    (void)node;
    /* String literal is a pointer (long / bit[64]) */
    return type_make_bitn(sema->arena, 64);
}

/* ------------------------------------------------------------------ */
/*  Type resolution                                                    */
/* ------------------------------------------------------------------ */

static HppType *sema_resolve_type(Sema *sema, AstNode *type_node)
{
    if (type_node == NULL) {
        return NULL;
    }

    switch (type_node->kind) {
    case NODE_TYPE_BIT: {
        uint32_t w = type_node->as.type_bit.width;
        if (w == 0) {
            /* plain 'bit' = bit[1] */
            return &HPP_TYPE_BIT;
        }
        return type_make_bitn(sema->arena, w);
    }

    case NODE_TYPE_NAMED: {
        const char *name = type_node->as.type_named.name;
        HppType *t = type_table_lookup(sema->types, name);
        if (t == NULL) {
            sema_error(sema, type_node->loc,
                       "undefined type '%s'", name);
        }
        return t;
    }

    default:
        sema_error(sema, type_node->loc,
                   "invalid type node: %s",
                   ast_node_kind_str(type_node->kind));
        return NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static void sema_check_condition(Sema *sema, AstNode *expr)
{
    if (expr == NULL) {
        return;
    }
    HppType *t = expr->resolved_type;
    if (t == NULL) {
        /* Unresolved literal — coerce to bit */
        sema_coerce_literal(sema, expr, &HPP_TYPE_BIT);
        t = expr->resolved_type;
    }
    if (t != NULL && t->bit_width != 1) {
        sema_error(sema, expr->loc,
                   "condition must be bit type, got %u bits",
                   t->bit_width);
    }
}

static bool sema_literal_fits(uint64_t value, uint32_t bits)
{
    if (bits >= 64) {
        return true;
    }
    return value < (1ULL << bits);
}

static bool sema_coerce_literal(Sema *sema, AstNode *node, HppType *target)
{
    if (node == NULL || target == NULL) {
        return false;
    }
    if (node->kind != NODE_INT_LIT) {
        return false;
    }
    if (node->resolved_type != NULL) {
        /* Already resolved */
        return type_equivalent(node->resolved_type, target);
    }

    uint64_t value = node->as.int_lit.value;
    if (!sema_literal_fits(value, target->bit_width)) {
        (void)sema; /* sema used for arena only here; caller reports error */
        return false;
    }

    node->resolved_type = target;
    return true;
}
