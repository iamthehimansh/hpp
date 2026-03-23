import std/{io, fmt};

def int  bit[32];
def long bit[64];

fn my_add(a: int, b: int) -> int { return a + b; }
fn my_mul(a: int, b: int) -> int { return a * b; }
fn my_sub(a: int, b: int) -> int { return a - b; }

fn apply(f: long, x: int, y: int) -> long {
    return f(x, y);
}

fn main() -> int {
    long fp_a = &my_add;
    long fp_m = &my_mul;
    long fp_s = &my_sub;

    long r1 = fp_a(10, 20);
    long r2 = fp_m(6, 7);
    long r3 = fp_s(50, 8);

    puts("add(10,20) = "); print_long(r1); print_char('\n');
    puts("mul(6,7)   = "); print_long(r2); print_char('\n');
    puts("sub(50,8)  = "); print_long(r3); print_char('\n');

    long r4 = apply(fp_a, 100, 200);
    long r5 = apply(fp_m, 3, 4);
    puts("apply(add, 100,200) = "); print_long(r4); print_char('\n');
    puts("apply(mul, 3,4)     = "); print_long(r5); print_char('\n');

    return 0;
}
