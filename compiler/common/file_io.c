#include "common/file_io.h"
#include <stdio.h>

char *file_read_all(Arena *arena, const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char *buf = arena_alloc(arena, (size_t)size + 1);
    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[read] = '\0';
    if (out_len) *out_len = read;
    return buf;
}
