#include "common/strtab.h"
#include <string.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 256
#define LOAD_FACTOR 0.75

static unsigned int fnv1a(const char *s, size_t len) {
    unsigned int hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (unsigned char)s[i];
        hash *= 16777619u;
    }
    return hash;
}

StrTab *strtab_create(Arena *arena) {
    StrTab *tab = arena_alloc(arena, sizeof(StrTab));
    tab->capacity = INITIAL_CAPACITY;
    tab->count = 0;
    tab->arena = arena;
    tab->entries = arena_alloc(arena, sizeof(StrTabEntry) * (size_t)tab->capacity);
    return tab;
}

static void strtab_grow(StrTab *tab) {
    int old_cap = tab->capacity;
    StrTabEntry *old_entries = tab->entries;

    tab->capacity = old_cap * 2;
    tab->entries = arena_alloc(tab->arena, sizeof(StrTabEntry) * (size_t)tab->capacity);
    tab->count = 0;

    for (int i = 0; i < old_cap; i++) {
        if (old_entries[i].str != NULL) {
            // Re-insert
            unsigned int idx = old_entries[i].hash & (unsigned int)(tab->capacity - 1);
            while (tab->entries[idx].str != NULL) {
                idx = (idx + 1) & (unsigned int)(tab->capacity - 1);
            }
            tab->entries[idx] = old_entries[i];
            tab->count++;
        }
    }
}

const char *strtab_intern(StrTab *tab, const char *s, size_t len) {
    if ((double)tab->count >= (double)tab->capacity * LOAD_FACTOR) {
        strtab_grow(tab);
    }

    unsigned int hash = fnv1a(s, len);
    unsigned int idx = hash & (unsigned int)(tab->capacity - 1);

    while (tab->entries[idx].str != NULL) {
        if (tab->entries[idx].hash == hash &&
            tab->entries[idx].len == len &&
            memcmp(tab->entries[idx].str, s, len) == 0) {
            return tab->entries[idx].str;
        }
        idx = (idx + 1) & (unsigned int)(tab->capacity - 1);
    }

    // Not found, intern it
    char *interned = arena_strndup(tab->arena, s, len);
    tab->entries[idx].str = interned;
    tab->entries[idx].len = len;
    tab->entries[idx].hash = hash;
    tab->count++;
    return interned;
}
