import std/fmt;
import std/mem;
import std/io;
import std/time;
import std/rand;

def int bit[32];
def long bit[64];

fn main() -> int {
    // print_int from std/fmt
    print_int(42);
    print_newline();

    // alloc/free from std/mem
    long buf = alloc(32);
    mem_set(buf, 65, 5);
    print_str(buf, 5);
    print_newline();
    free(buf, 32);

    // sleep_ms from std/time
    sleep_ms(50);

    // random_int from std/rand
    int r = random_int();
    print_int(r);
    print_newline();

    return 0;
}
