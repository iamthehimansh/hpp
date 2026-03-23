import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];

fn main() -> int {
    // ---- Form 1: int arr[n]; ----
    int nums[5];
    nums[0] = 10;
    nums[1] = 20;
    nums[2] = 30;
    nums[3] = 40;
    nums[4] = 50;
    puts("Form 1: ");
    int_print(nums, 5);

    // ---- Form 2: int arr[] = [values]; ----
    int primes[] = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29];
    puts("Form 2: ");
    int_print(primes, 10);

    // ---- Form 3: int arr[n] = [values]; ----
    int fib[8] = [1, 1, 2, 3, 5, 8, 13, 21];
    puts("Form 3: ");
    int_print(fib, 8);

    // ---- Use [] for read/write ----
    int sum = 0;
    for (int i = 0; i < 10; i = i + 1) {
        sum = sum + primes[i];
    }
    puts("Sum of primes: ");
    print_int(sum);
    print_char('\n');

    // ---- Modify and sort ----
    fib[7] = 0;
    fib[6] = 0;
    int_sort(fib, 8);
    puts("Sorted: ");
    int_print(fib, 8);

    // ---- Byte array ----
    byte hello[] = [72, 101, 108, 108, 111];
    puts("Bytes: ");
    print_str(hello, 5);
    print_char('\n');

    // ---- Expressions as size ----
    int n = 3;
    int dynamic[n * 2];
    for (int i = 0; i < 6; i = i + 1) {
        dynamic[i] = i * i;
    }
    puts("Dynamic: ");
    int_print(dynamic, 6);

    // Cleanup
    int_free(nums, 5);
    int_free(primes, 10);
    int_free(fib, 8);
    byte_free(hello, 5);
    int_free(dynamic, 6);

    return 0;
}
