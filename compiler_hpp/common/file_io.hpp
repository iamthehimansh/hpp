// file_io.c rewritten in H++
// Provides file_read_all(arena, path, out_len) -> pointer
// Uses raw syscalls instead of libc fopen/fread

import std/{sys, mem};

def int  bit[32];
def long bit[64];

// External C function from arena.c (linked via gcc)
fn arena_alloc(arena: long, size: long) -> long;

fn file_read_all(arena: long, path: long, out_len: long) -> long {
    // Open file: O_RDONLY = 0
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        return 0;
    }

    // Seek to end to get size: SEEK_END = 2
    long size = sys_lseek(fd, 0, 2);
    if (size < 0) {
        sys_close(fd);
        return 0;
    }

    // Seek back to start: SEEK_SET = 0
    sys_lseek(fd, 0, 0);

    // Allocate buffer from arena (size + 1 for null terminator)
    long buf = arena_alloc(arena, size + 1);

    // Read entire file
    long bytes_read = sys_read(fd, buf, size);
    sys_close(fd);

    // Null-terminate
    if (bytes_read < 0) {
        bytes_read = 0;
    }
    mem_write8(buf, int(bytes_read), 0);

    // Store length if out_len is not null
    if (out_len != 0) {
        *out_len = bytes_read;
    }

    return buf;
}
