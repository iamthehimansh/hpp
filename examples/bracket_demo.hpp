import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];

fn main() -> int {
    long a = int_new(5);

    // Write using []
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;
    a[4] = 50;

    // Read using []
    println("Array values:");
    for (int i = 0; i < 5; i = i + 1) {
        puts("  a[");
        print_int(i);
        puts("] = ");
        print_int(a[i]);
        print_char('\n');
    }

    // Use in expressions
    int sum = a[0] + a[1] + a[2] + a[3] + a[4];
    puts("Sum: ");
    print_int(sum);
    print_char('\n');

    // Use in conditions
    if (a[2] > a[1]) {
        println("a[2] > a[1]");
    }

    // Modify via []
    a[2] = a[2] * 2;
    puts("a[2] doubled: ");
    print_int(a[2]);
    print_char('\n');

    // Print with int_print
    puts("Final: ");
    int_print(a, 5);

    int_free(a, 5);
    return 0;
}
