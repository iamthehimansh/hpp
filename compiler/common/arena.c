#include "common/arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_ARENA_SIZE (1024 * 1024)  // 1MB
#define ALIGNMENT 8

static size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

static ArenaChunk *chunk_create(size_t capacity) {
    ArenaChunk *chunk = malloc(sizeof(ArenaChunk));
    if (!chunk) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }
    chunk->base = malloc(capacity);
    if (!chunk->base) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }
    chunk->used = 0;
    chunk->capacity = capacity;
    chunk->next = NULL;
    return chunk;
}

Arena *arena_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = DEFAULT_ARENA_SIZE;
    Arena *arena = malloc(sizeof(Arena));
    if (!arena) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }
    arena->default_capacity = initial_capacity;
    arena->first = chunk_create(initial_capacity);
    arena->current = arena->first;
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    size = align_up(size, ALIGNMENT);
    if (arena->current->used + size > arena->current->capacity) {
        size_t new_cap = arena->default_capacity;
        if (size > new_cap) new_cap = size * 2;
        ArenaChunk *chunk = chunk_create(new_cap);
        arena->current->next = chunk;
        arena->current = chunk;
    }
    void *ptr = arena->current->base + arena->current->used;
    arena->current->used += size;
    memset(ptr, 0, size);
    return ptr;
}

char *arena_strdup(Arena *arena, const char *s) {
    size_t len = strlen(s);
    char *copy = arena_alloc(arena, len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

char *arena_strndup(Arena *arena, const char *s, size_t len) {
    char *copy = arena_alloc(arena, len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

void arena_destroy(Arena *arena) {
    ArenaChunk *chunk = arena->first;
    while (chunk) {
        ArenaChunk *next = chunk->next;
        free(chunk->base);
        free(chunk);
        chunk = next;
    }
    free(arena);
}
