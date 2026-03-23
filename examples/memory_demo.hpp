import std/{fmt, io, mem, str};

def int bit[32];
def long bit[64];

fn main() -> int {
    // Allocate 64 bytes
    long buf = alloc(64);
    if (buf == 0) {
        print_str(buf, 0);
        return 1;
    }

    // Print success message using print_int
    print_int(1);
    print_char(58);
    print_char(32);

    // Print the pointer value
    print_hex(buf);
    print_newline();

    // Test mem_set: fill first 10 bytes with 'A' (65)
    mem_set(buf, 65, 10);

    // Print those 10 bytes
    print_str(buf, 10);
    print_newline();

    // Test str_len (the 10 A's followed by zero from alloc)
    long len = str_len(buf);
    print_int(10);
    print_char(61);
    print_long(len);
    print_newline();

    // Free memory
    free(buf, 64);

    // Test itoa
    long buf2 = alloc(32);
    long digits = itoa(12345, buf2);
    print_str(buf2, digits);
    print_newline();
    free(buf2, 32);

    return 0;
}
