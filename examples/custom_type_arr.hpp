import std/{io, fmt, arr};

def int   bit[32];
def long  bit[64];
def byte  bit[8];
def short bit[16];
def bool  bit[8];

fn main() -> int {
    // ---- Custom type: short (16-bit) → maps to int backend (≤32 bits) ----
    short scores[4] = [100, 200, 300, 400];
    puts("short[]: ");
    // int_print works since shorts are stored as ints
    int_print(scores, 4);

    scores[2] = 999;
    puts("scores[2] = ");
    print_int(scores[2]);
    print_char('\n');

    // ---- Custom type: bool (8-bit) → maps to byte backend (≤8 bits) ----
    bool flags[3] = [1, 0, 1];
    puts("flags: ");
    print_int(flags[0]);
    print_char(' ');
    print_int(flags[1]);
    print_char(' ');
    print_int(flags[2]);
    print_char('\n');

    // ---- String as byte array ----
    long greeting = "Hello, World!";
    puts("String[0] = ");
    print_char(greeting[0]);     // 'H' — byte_get since greeting is a string var
    print_char('\n');

    puts("String[7] = ");
    print_char(greeting[7]);     // 'W'
    print_char('\n');

    // Iterate string chars
    puts("Chars: ");
    for (int i = 0; i < 13; i = i + 1) {
        int ch = greeting[i];
        print_char(ch);
        print_char(' ');
    }
    print_char('\n');

    // ---- Modify string copy ----
    byte buf[20];
    for (int i = 0; i < 13; i = i + 1) {
        buf[i] = greeting[i];
    }
    buf[0] = 'J';           // Change H → J
    buf[13] = 0;             // null terminate
    puts("Modified: ");
    puts(buf);
    print_char('\n');

    // Cleanup
    int_free(scores, 4);
    byte_free(flags, 3);
    byte_free(buf, 20);

    return 0;
}
