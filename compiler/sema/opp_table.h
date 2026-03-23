#ifndef HPP_OPP_TABLE_H
#define HPP_OPP_TABLE_H

#include "common/arena.h"
#include "sema/types.h"
#include "ast/ast.h"

typedef struct {
    HppType *left_type;
    HppType *right_type;
    HppType *result_type;
    BinOp binop;
    UnOp unop;
    bool is_unary;
    AstNodeList *asm_blocks;
} OppEntry;

typedef struct OppTable OppTable;

OppTable  *opp_table_create(Arena *arena);
void       opp_table_register(OppTable *table, OppEntry *entry);
OppEntry  *opp_table_lookup_binary(OppTable *table, HppType *left, BinOp op, HppType *right);
OppEntry  *opp_table_lookup_unary(OppTable *table, UnOp op, HppType *operand);

#endif
