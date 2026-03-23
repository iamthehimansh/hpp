import std/{io, fmt, mem};

def int  bit[32];
def long bit[64];

fn main() -> int {
    // ---- Basic: &var and *ptr ----
    int x = 42;
    long ptr = &x;

    puts("x     = "); print_int(x); print_char('\n');
    puts("*ptr  = "); print_long(*ptr); print_char('\n');

    // ---- Write through pointer ----
    *ptr = 99;
    puts("After *ptr = 99:\n");
    puts("x     = "); print_int(x); print_char('\n');
    puts("*ptr  = "); print_long(*ptr); print_char('\n');

    // ---- Swap using pointers ----
    int a = 10;
    int b = 20;
    puts("\nBefore swap: a="); print_int(a); puts(" b="); print_int(b); print_char('\n');
    swap(&a, &b);
    puts("After swap:  a="); print_int(a); puts(" b="); print_int(b); print_char('\n');

    // ---- Heap pointer ----
    long hp = alloc(8);
    *hp = 12345;
    puts("\nHeap: *hp = "); print_long(*hp); print_char('\n');
    free(hp, 8);

    // ---- Double pointer ----
    long pp = &ptr;
    puts("\nptr   = "); print_hex(ptr); print_char('\n');
    puts("*pp   = "); print_hex(*pp); print_char('\n');   // same as ptr
    puts("**pp  = "); print_long(**pp); print_char('\n');  // same as *ptr = x

    return 0;
}

fn swap(a: long, b: long) {
    long tmp = *a;
    *a = *b;
    *b = tmp;
}
