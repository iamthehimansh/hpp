def int bit[32];
def long bit[64];

fn main() -> int {
    // Write "Hi!\n" to stdout using inline asm to set up the data
    asm linux {
        ; push "Hi!\n" onto stack
        mov dword [rsp-4], 0x0a214948
        lea rsi, [rsp-4]
        mov rax, 1
        mov rdi, 1
        mov rdx, 4
        syscall
    }
    return 0;
}
