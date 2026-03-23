import std/{fmt, io};
import mathlib;

def int bit[32];

fn main() -> int {
    // square
    int s = square(7);
    print_int(s);
    print_char(10);

    // cube
    int c = cube(5);
    print_int(c);
    print_char(10);

    // max / min
    int big = max(42, 17);
    int small = min(42, 17);
    print_int(big);
    print_char(32);
    print_int(small);
    print_char(10);

    // clamp
    int clamped = clamp(100, 0, 50);
    print_int(clamped);
    print_char(10);

    // factorial
    int f = factorial(10);
    print_int(f);
    print_char(10);

    return 0;
}
