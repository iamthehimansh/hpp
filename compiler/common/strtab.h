#ifndef HPP_STRTAB_H
#define HPP_STRTAB_H

#include "common/arena.h"
#include <stddef.h>

typedef struct StrTabEntry {
    const char *str;
    size_t len;
    unsigned int hash;
} StrTabEntry;

typedef struct StrTab {
    StrTabEntry *entries;
    int capacity;
    int count;
    Arena *arena;
} StrTab;

StrTab     *strtab_create(Arena *arena);
const char *strtab_intern(StrTab *tab, const char *s, size_t len);

#endif
