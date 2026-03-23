#ifndef HPP_FILE_IO_H
#define HPP_FILE_IO_H

#include "common/arena.h"
#include <stddef.h>

// Read entire file into arena-allocated buffer. Returns NULL on failure.
char *file_read_all(Arena *arena, const char *path, size_t *out_len);

#endif
