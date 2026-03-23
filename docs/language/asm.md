# Inline Assembly

H++ supports inline assembly blocks for direct hardware access.

## Syntax

```c
asm linux {
    mov rax, 1          ; sys_write
    mov rdi, 1          ; stdout
    syscall
}
```

Only the block matching the compilation target is compiled. Others are ignored entirely (not even parsed).

## Targets

| Target | Syntax |
|--------|--------|
| `asm linux { }` | Linux x86-64 |
| `asm windows { }` | Windows x86-64 (future) |
| `asm macos { }` | macOS (future) |

## Assembly Syntax

Uses NASM Intel syntax inside the block. Assembly comments use `;`.

## Register Access

You have direct access to all CPU registers. The compiler does NOT save/restore registers — you are responsible.

## Example: System Call

```c
fn sys_write_direct(fd: int, buf: long, len: long) -> long {
    asm linux {
        mov rax, 1
        mov rdi, [rbp - 8]     ; fd
        mov rsi, [rbp - 16]    ; buf
        mov rdx, [rbp - 24]    ; len
        syscall
    }
}
```

## Variable Access

Function parameters and local variables are on the stack at `[rbp - offset]`. Each variable occupies 8 bytes. First parameter is at `[rbp - 8]`, second at `[rbp - 16]`, etc.
