#include "sema/opp_table.h"
#include <string.h>

#define OPP_TABLE_INIT_CAP 32

struct OppTable {
    Arena    *arena;
    OppEntry *entries;
    int       count;
    int       capacity;
};

OppTable *opp_table_create(Arena *arena)
{
    OppTable *t = arena_alloc(arena, sizeof(OppTable));
    t->arena    = arena;
    t->capacity = OPP_TABLE_INIT_CAP;
    t->count    = 0;
    t->entries  = arena_alloc(arena, (size_t)t->capacity * sizeof(OppEntry));
    return t;
}

void opp_table_register(OppTable *table, OppEntry *entry)
{
    if (table->count >= table->capacity) {
        int new_cap = table->capacity * 2;
        OppEntry *new_entries = arena_alloc(table->arena,
                                            (size_t)new_cap * sizeof(OppEntry));
        memcpy(new_entries, table->entries,
               (size_t)table->count * sizeof(OppEntry));
        table->entries  = new_entries;
        table->capacity = new_cap;
    }
    table->entries[table->count] = *entry;
    table->count++;
}

OppEntry *opp_table_lookup_binary(OppTable *table, HppType *left,
                                  BinOp op, HppType *right)
{
    for (int i = 0; i < table->count; i++) {
        OppEntry *e = &table->entries[i];
        if (e->is_unary) {
            continue;
        }
        if (e->binop == op &&
            type_equivalent(e->left_type, left) &&
            type_equivalent(e->right_type, right)) {
            return e;
        }
    }
    return NULL;
}

OppEntry *opp_table_lookup_unary(OppTable *table, UnOp op, HppType *operand)
{
    for (int i = 0; i < table->count; i++) {
        OppEntry *e = &table->entries[i];
        if (!e->is_unary) {
            continue;
        }
        if (e->unop == op && type_equivalent(e->left_type, operand)) {
            return e;
        }
    }
    return NULL;
}
