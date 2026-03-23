#ifndef HPP_SEMA_H
#define HPP_SEMA_H

#include "common/arena.h"
#include "common/strtab.h"
#include "sema/types.h"
#include "sema/symtab.h"
#include "sema/opp_table.h"
#include "ast/ast.h"

typedef struct {
    Arena *arena;
    StrTab *strtab;
    TypeTable *types;
    SymTable *symbols;
    OppTable *opps;
    const char *filename;
    HppType *current_func_return_type;
    int loop_depth;
    bool had_error;
    bool is_library;    /* true = skip main() check, for .hpp library modules */
} Sema;

void sema_init(Sema *sema, Arena *arena, StrTab *strtab, const char *filename);
bool sema_analyze(Sema *sema, AstNode *program);

/* Register external function declarations from a parsed .hdef module */
void sema_register_module(Sema *sema, AstNode *module_program);

#endif
