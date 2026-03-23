import std/{io, fmt, mem};

def int  bit[32];
def long bit[64];

fn main() -> int {
    // ---- Get address of stack variable ----
    int x = 42;
    long ptr = &x;

    puts("x = ");
    print_int(x);
    print_char('\n');

    puts("&x = ");
    print_hex(ptr);
    print_char('\n');

    // ---- Read value through pointer ----
    int val = mem_read32(ptr, 0);
    puts("*(&x) = ");
    print_int(val);
    print_char('\n');

    // ---- Write through pointer ----
    mem_write32(ptr, 0, 99);
    puts("After *(&x) = 99, x = ");
    print_int(x);
    print_char('\n');

    // ---- Pass address to function ----
    int a = 10;
    int b = 20;
    swap(&a, &b);
    puts("After swap: a=");
    print_int(a);
    puts(" b=");
    print_int(b);
    print_char('\n');

    // ---- Array of values via pointer ----
    int arr0 = 100;
    int arr1 = 200;
    int arr2 = 300;
    long base = &arr0;
    // Stack vars are 8 bytes apart (each slot is 8 bytes)
    puts("base[0] = ");
    print_int(mem_read32(base, 0));
    print_char('\n');

    return 0;
}

fn swap(a: long, b: long) {
    int tmp = mem_read32(a, 0);
    mem_write32(a, 0, mem_read32(b, 0));
    mem_write32(b, 0, tmp);
}
