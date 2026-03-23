import std/{io, mem, sys};

def int bit[32];
def long bit[64];

fn make_path(buf: long) {
    // Build "/tmp/hpp.txt" in memory
    // /=47 t=116 m=109 p=112 /=47 h=104 p=112 p=112 .=46 t=116 x=120 t=116
    asm linux {
        mov rdi, [rbp - 8]
        mov byte [rdi+0], 47
        mov byte [rdi+1], 116
        mov byte [rdi+2], 109
        mov byte [rdi+3], 112
        mov byte [rdi+4], 47
        mov byte [rdi+5], 104
        mov byte [rdi+6], 112
        mov byte [rdi+7], 112
        mov byte [rdi+8], 46
        mov byte [rdi+9], 116
        mov byte [rdi+10], 120
        mov byte [rdi+11], 116
        mov byte [rdi+12], 0
    }
}

fn make_msg(buf: long) {
    // Build "Hello from H++!\n" in memory
    asm linux {
        mov rdi, [rbp - 8]
        mov byte [rdi+0], 72
        mov byte [rdi+1], 101
        mov byte [rdi+2], 108
        mov byte [rdi+3], 108
        mov byte [rdi+4], 111
        mov byte [rdi+5], 32
        mov byte [rdi+6], 102
        mov byte [rdi+7], 114
        mov byte [rdi+8], 111
        mov byte [rdi+9], 109
        mov byte [rdi+10], 32
        mov byte [rdi+11], 72
        mov byte [rdi+12], 43
        mov byte [rdi+13], 43
        mov byte [rdi+14], 33
        mov byte [rdi+15], 10
        mov byte [rdi+16], 0
    }
}

fn main() -> int {
    long path = alloc(32);
    long msg = alloc(32);
    long readbuf = alloc(64);
    mem_zero(path, 32);
    mem_zero(msg, 32);
    mem_zero(readbuf, 64);

    make_path(path);
    make_msg(msg);

    // Write to file
    int fd = file_open_write(path);
    file_write(fd, msg, 16);
    file_close(fd);

    // Read it back
    int fd2 = file_open_read(path);
    long n = file_read(fd2, readbuf, 64);
    file_close(fd2);

    // Print what we read
    print_str(readbuf, n);

    // Delete the file
    sys_unlink(path);

    // Clean up
    free(path, 32);
    free(msg, 32);
    free(readbuf, 64);

    return 0;
}
