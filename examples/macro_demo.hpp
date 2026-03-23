import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];

// ---- Simple macros ----
macro log(msg) { println(msg) }
macro get(a, i) { int_get(a, i) }
macro set(a, i, v) { int_set(a, i, v) }
macro debug(label, val) { puts(label); puts(": "); print_int(val); print_char('\n') }

fn main() -> int {
    log("=== Macro Demo ===");

    // Array with macro access
    long nums = int_new(5);
    set(nums, 0, 10);
    set(nums, 1, 20);
    set(nums, 2, 30);
    set(nums, 3, 40);
    set(nums, 4, 50);

    puts("Array: ");
    int_print(nums, 5);

    // Read with macro
    debug("Element 2", get(nums, 2));
    debug("Element 4", get(nums, 4));

    // Sum using normal loop + macro get
    int total = 0;
    for (int i = 0; i < 5; i = i + 1) {
        total = total + get(nums, i);
    }
    debug("Sum", total);

    // String literals
    long greeting = "Hello from macros!";
    println(greeting);

    // Char literals
    print_char('H');
    print_char('+');
    print_char('+');
    print_char(' ');
    print_char('r');
    print_char('o');
    print_char('c');
    print_char('k');
    print_char('s');
    print_char('!');
    print_char('\n');

    int_free(nums, 5);
    log("=== Done ===");
    return 0;
}
