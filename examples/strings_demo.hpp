import std/{io, fmt, str, mem, sys};

def int  bit[32];
def long bit[64];

fn main() -> int {
    // ---- String literals ----
    println("Hello, World!");
    println("H++ now has string literals!");

    // ---- Char literals ----
    print_char('H');
    print_char('+');
    print_char('+');
    print_char('\n');

    // ---- puts vs println ----
    puts("no newline here...");
    println(" but here!");

    // ---- String functions work with literals ----
    long greeting = "Hello";
    long target = "World";
    long glen = str_len(greeting);
    long tlen = str_len(target);

    puts("Greeting length: ");
    print_long(glen);
    print_newline();

    // ---- Concat two strings ----
    long buf = alloc(64);
    mem_zero(buf, 64);
    str_copy(buf, greeting);
    str_cat(buf, ", ");
    str_cat(buf, target);
    str_cat(buf, "!");
    puts("Combined: ");
    println(buf);
    free(buf, 64);

    // ---- Escape sequences ----
    puts("Tab:\there\n");
    puts("Backslash: \\\n");

    // ---- Use in file I/O ----
    int fd = file_open_write("/tmp/hpp_string_test.txt");
    long msg = "Written by H++!\n";
    long mlen = str_len(msg);
    file_write(fd, msg, mlen);
    file_close(fd);

    // Read back
    int fd2 = file_open_read("/tmp/hpp_string_test.txt");
    long rbuf = alloc(64);
    long n = file_read(fd2, rbuf, 64);
    file_close(fd2);
    puts("From file: ");
    print_str(rbuf, n);
    free(rbuf, 64);

    // Cleanup
    sys_unlink("/tmp/hpp_string_test.txt");

    println("Done!");
    return 0;
}
