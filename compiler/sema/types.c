#include "sema/types.h"
#include <string.h>

HppType HPP_TYPE_BIT  = { .name = "bit",  .bit_width = 1, .is_bit_array = false };
HppType HPP_TYPE_VOID = { .name = NULL,   .bit_width = 0, .is_bit_array = false };

#define TYPE_TABLE_INIT_CAP 64

typedef struct TypeEntry {
    const char *name;   /* interned pointer — used as hash key */
    HppType    *type;
} TypeEntry;

struct TypeTable {
    Arena     *arena;
    TypeEntry *entries;
    int        count;
    int        capacity;
};

/* --------------- hash helpers (open addressing) --------------- */

static unsigned int hash_str(const char *s)
{
    unsigned int h = 2166136261u;
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 16777619u;
    }
    return h;
}

static int find_slot(TypeEntry *entries, int capacity, const char *name)
{
    unsigned int h = hash_str(name);
    int idx = (int)(h & (unsigned int)(capacity - 1));
    for (;;) {
        if (entries[idx].name == NULL) {
            return idx;
        }
        if (strcmp(entries[idx].name, name) == 0) {
            return idx;
        }
        idx = (idx + 1) & (capacity - 1);
    }
}

static void grow_table(TypeTable *table)
{
    int new_cap = table->capacity * 2;
    TypeEntry *new_entries = arena_alloc(table->arena,
                                         (size_t)new_cap * sizeof(TypeEntry));
    memset(new_entries, 0, (size_t)new_cap * sizeof(TypeEntry));

    for (int i = 0; i < table->capacity; i++) {
        if (table->entries[i].name != NULL) {
            int slot = find_slot(new_entries, new_cap, table->entries[i].name);
            new_entries[slot] = table->entries[i];
        }
    }
    table->entries  = new_entries;
    table->capacity = new_cap;
}

/* --------------- public API --------------- */

TypeTable *type_table_create(Arena *arena)
{
    TypeTable *t = arena_alloc(arena, sizeof(TypeTable));
    t->arena    = arena;
    t->capacity = TYPE_TABLE_INIT_CAP;
    t->count    = 0;
    t->entries  = arena_alloc(arena, (size_t)t->capacity * sizeof(TypeEntry));
    memset(t->entries, 0, (size_t)t->capacity * sizeof(TypeEntry));
    return t;
}

void type_table_init_builtins(TypeTable *table)
{
    int slot = find_slot(table->entries, table->capacity, HPP_TYPE_BIT.name);
    table->entries[slot].name = HPP_TYPE_BIT.name;
    table->entries[slot].type = &HPP_TYPE_BIT;
    table->count++;
}

HppType *type_table_define(TypeTable *table, const char *name, uint32_t bit_width)
{
    /* check for duplicate */
    int slot = find_slot(table->entries, table->capacity, name);
    if (table->entries[slot].name != NULL) {
        return NULL;  /* already exists */
    }

    HppType *t = arena_alloc(table->arena, sizeof(HppType));
    t->name        = name;
    t->bit_width   = bit_width;
    t->is_bit_array = false;

    table->entries[slot].name = name;
    table->entries[slot].type = t;
    table->count++;

    if (table->count * 2 > table->capacity) {
        grow_table(table);
    }
    return t;
}

HppType *type_table_lookup(TypeTable *table, const char *name)
{
    int slot = find_slot(table->entries, table->capacity, name);
    if (table->entries[slot].name == NULL) {
        return NULL;
    }
    return table->entries[slot].type;
}

HppType *type_make_bitn(Arena *arena, uint32_t n)
{
    HppType *t = arena_alloc(arena, sizeof(HppType));
    t->name        = NULL;
    t->bit_width   = n;
    t->is_bit_array = true;
    return t;
}

bool type_equivalent(HppType *a, HppType *b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return a->bit_width == b->bit_width;
}

bool type_can_widen(HppType *from, HppType *to)
{
    if (from == NULL || to == NULL) return false;
    return from->bit_width <= to->bit_width;
}

int type_reg_size(uint32_t bit_width)
{
    if (bit_width <= 8)  return 8;
    if (bit_width <= 16) return 16;
    if (bit_width <= 32) return 32;
    return 64;
}

int type_alignment(uint32_t bit_width)
{
    if (bit_width <= 8)  return 1;
    if (bit_width <= 16) return 2;
    if (bit_width <= 32) return 4;
    return 8;
}
