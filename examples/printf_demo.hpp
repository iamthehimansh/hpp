import std/{io, fmt, printf};

def int  bit[32];
def long bit[64];

fn main() -> int {
    // Basic format specifiers
    fmt_out("Hello %s! You are %d years old.\n", "World", 25, 0, 0);

    // Multiple strings
    fmt_out("%s + %s = %s\n", "one", "two", "three", 0);

    // Numbers
    fmt_out("dec=%d hex=%x char=%c\n", 255, 255, 65, 0);

    // To stderr
    fmt_err("error: %s at line %d\n", "something broke", 42, 0, 0);

    // Percent literal
    fmt_out("100%% done!\n", 0, 0, 0, 0);

    // %.*s (precision string)
    fmt_out("first 5: %.*s\n", 5, "Hello World", 0, 0);

    return 0;
}
