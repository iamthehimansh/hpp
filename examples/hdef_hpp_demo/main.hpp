import std/{fmt, io};
import utils;

fn main() -> int {
    // add
    print_int(add(100, 200));
    print_char(10);

    // multiply
    print_int(multiply(6, 7));
    print_char(10);

    // power
    print_int(power(2, 10));
    print_char(10);

    // sum_to
    print_int(sum_to(100));
    print_char(10);

    return 0;
}
