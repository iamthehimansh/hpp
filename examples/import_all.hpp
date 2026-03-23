import std;

def int bit[32];
def long bit[64];

fn main() -> int {
    print_int(999);
    print_newline();

    int pid = sys_getpid();
    print_int(pid);
    print_newline();

    return 0;
}
