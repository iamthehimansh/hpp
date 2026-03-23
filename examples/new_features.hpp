import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn main(argc: int, argv: long) -> int {
    // ---- argc/argv ----
    puts("argc = ");
    print_int(argc);
    print_char('\n');

    // ---- ++ and -- ----
    int x = 0;
    x++;
    x++;
    x++;
    puts("After 3x ++: ");
    print_int(x);
    print_char('\n');

    x--;
    puts("After --: ");
    print_int(x);
    print_char('\n');

    // ---- += -= *= ----
    int y = 10;
    y += 5;
    puts("10 += 5 = ");
    print_int(y);
    print_char('\n');

    y *= 2;
    puts("*= 2 = ");
    print_int(y);
    print_char('\n');

    y -= 3;
    puts("-= 3 = ");
    print_int(y);
    print_char('\n');

    // ---- switch/case ----
    int val = 2;
    switch (val) {
        case 1:
            println("one");
            break;
        case 2:
            println("two");
            break;
        case 3:
            println("three");
            break;
        default:
            println("other");
            break;
    }

    // ---- null ----
    long ptr = null;
    if (ptr == 0) {
        println("null works!");
    }

    // ---- sizeof ----
    puts("sizeof(int) = ");
    print_int(sizeof(int));
    print_char('\n');

    puts("sizeof(long) = ");
    print_int(sizeof(long));
    print_char('\n');

    puts("sizeof(byte) = ");
    print_int(sizeof(byte));
    print_char('\n');

    // ---- for with ++ ----
    puts("Count: ");
    for (int i = 0; i < 5; i++) {
        print_int(i);
        print_char(' ');
    }
    print_char('\n');

    return 0;
}
