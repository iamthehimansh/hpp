import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn main() -> int {
    // ---- byte array ----
    byte msg[] = ['H', 'e', 'l', 'l', 'o'];
    msg[4] = '!';                    // byte_set
    print_str(msg, 5);
    print_char(' ');
    int ch = msg[0];                 // byte_get → 72
    print_int(ch);
    print_char('\n');

    // ---- int array ----
    int nums[] = [100, 200, 300];
    nums[1] = nums[0] + nums[2];    // int_set, int_get
    puts("ints: ");
    int_print(nums, 3);

    // ---- long array ----
    long addrs[] = [1000, 2000, 3000];
    addrs[0] = addrs[1] + addrs[2]; // long_set, long_get
    puts("long[0] = ");
    print_long(addrs[0]);            // long_get
    print_char('\n');

    // ---- mixed in expressions ----
    int a[] = [10, 20, 30, 40, 50];
    int sum = 0;
    for (int i = 0; i < 5; i = i + 1) {
        sum = sum + a[i];            // int_get (a declared as int)
    }
    puts("sum = ");
    print_int(sum);
    print_char('\n');

    // Cleanup
    byte_free(msg, 5);
    int_free(nums, 3);
    long_free(addrs, 3);
    int_free(a, 5);

    return 0;
}
