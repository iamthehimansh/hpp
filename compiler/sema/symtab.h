#ifndef HPP_SYMTAB_H
#define HPP_SYMTAB_H

#include "common/arena.h"
#include "common/error.h"
#include "sema/types.h"
#include "ast/ast.h"

typedef enum { SYM_VAR, SYM_CONST, SYM_FUNC, SYM_PARAM } SymKind;

typedef struct Symbol {
    const char *name;
    SymKind kind;
    HppType *type;
    SourceLoc loc;
    bool is_mutable;
    int scope_depth;
    int stack_offset;
    int reg_index;
    /* For functions */
    struct {
        HppType **param_types;
        int param_count;
        HppType *return_type;
    } func_info;
    bool is_external;  /* std_ll functions */
} Symbol;

typedef struct Scope {
    struct Scope *parent;
    int depth;
    Symbol **symbols;
    int sym_capacity;
    int sym_count;
} Scope;

typedef struct SymTable {
    Scope *current;
    Arena *arena;
} SymTable;

SymTable *symtab_create(Arena *arena);
void      symtab_enter_scope(SymTable *tab);
void      symtab_exit_scope(SymTable *tab);
Symbol   *symtab_define(SymTable *tab, const char *name, SymKind kind, HppType *type, SourceLoc loc);
Symbol   *symtab_lookup(SymTable *tab, const char *name);
Symbol   *symtab_lookup_local(SymTable *tab, const char *name);
int       symtab_depth(SymTable *tab);

#endif
