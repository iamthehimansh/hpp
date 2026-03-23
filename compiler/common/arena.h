#ifndef HPP_ARENA_H
#define HPP_ARENA_H

#include <stddef.h>

typedef struct ArenaChunk {
    char *base;
    size_t used;
    size_t capacity;
    struct ArenaChunk *next;
} ArenaChunk;

typedef struct Arena {
    ArenaChunk *current;
    ArenaChunk *first;
    size_t default_capacity;
} Arena;

Arena *arena_create(size_t initial_capacity);
void  *arena_alloc(Arena *arena, size_t size);
char  *arena_strdup(Arena *arena, const char *s);
char  *arena_strndup(Arena *arena, const char *s, size_t len);
void   arena_destroy(Arena *arena);

#endif
