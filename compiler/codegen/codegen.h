#ifndef HPP_CODEGEN_H
#define HPP_CODEGEN_H

#include "common/arena.h"
#include "sema/types.h"
#include "sema/symtab.h"
#include "sema/opp_table.h"
#include "ast/ast.h"

typedef struct LoopCtx {
    int break_label;
    int continue_label;
    struct LoopCtx *parent;
} LoopCtx;

typedef struct {
    Arena *arena;
    TypeTable *types;
    SymTable *symbols;
    OppTable *opps;
    const char *target;
    char *output;
    size_t output_len;
    size_t output_cap;
    int label_counter;
    int stack_size;
    LoopCtx *loop_ctx;
    const char *current_func_name;
    bool is_library;    /* true = no _start, export all functions */
} CodeGen;

void  codegen_init(CodeGen *cg, Arena *arena, TypeTable *types,
                   SymTable *symbols, OppTable *opps, const char *target);
char *codegen_generate(CodeGen *cg, AstNode *program);

#endif
