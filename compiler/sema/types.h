#ifndef HPP_TYPES_H
#define HPP_TYPES_H

#include "common/arena.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct HppType {
    const char *name;       /* interned, or NULL for anonymous bit[n] */
    uint32_t bit_width;
    bool is_bit_array;      /* true for bare bit[n] */
} HppType;

typedef struct TypeTable TypeTable;

TypeTable *type_table_create(Arena *arena);
void       type_table_init_builtins(TypeTable *table);
HppType   *type_table_define(TypeTable *table, const char *name, uint32_t bit_width);
HppType   *type_table_lookup(TypeTable *table, const char *name);
HppType   *type_make_bitn(Arena *arena, uint32_t n);
bool       type_equivalent(HppType *a, HppType *b);
/* Returns true if 'from' can widen into 'to' (from <= to in bit width) */
bool       type_can_widen(HppType *from, HppType *to);
int        type_reg_size(uint32_t bit_width);
int        type_alignment(uint32_t bit_width);

extern HppType HPP_TYPE_BIT;
extern HppType HPP_TYPE_VOID;

#endif
