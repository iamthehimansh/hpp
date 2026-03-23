import std;

def int bit[32];
def long bit[64];

fn main() -> int {
    // Print numbers
    print_int(42);
    print_char(10);

    print_int(123);
    print_char(10);

    // Print hex
    print_hex(255);
    print_char(10);

    // Random number
    int r = random_int();
    print_int(r);
    print_char(10);

    // Get PID
    int pid = sys_getpid();
    print_int(pid);
    print_char(10);

    // Get time
    long t = get_time_sec();
    print_long(t);
    print_char(10);

    return 0;
}
