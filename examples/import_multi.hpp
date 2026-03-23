import std/{fmt, io, mem, time, rand, proc};

def int bit[32];
def long bit[64];

fn main() -> int {
    print_int(100);
    print_newline();

    long buf = alloc(16);
    mem_zero(buf, 16);
    free(buf, 16);

    print_int(200);
    print_newline();

    return 0;
}
