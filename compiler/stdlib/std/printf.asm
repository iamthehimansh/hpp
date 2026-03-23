; ============================================================
; H++ Printf-like Formatter (std/printf)
;
; Uses an argument array on stack for easy indexed access.
; Format specifiers: %s %d %u %c %x %%
; ============================================================

section .bss
_fmt_numbuf: resb 32

section .text

; ============================================================
; fmt_fd(fd: int, fmt: long, a0: long, a1: long, a2: long, a3: long)
; ============================================================
global fmt_fd
fmt_fd:
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rbp
    mov rbp, rsp

    ; Store args in a contiguous array at [rbp-32]
    ; a0 at [rbp-8], a1 at [rbp-16], a2 at [rbp-24], a3 at [rbp-32]
    sub rsp, 48
    mov [rbp - 8],  rdx         ; a0
    mov [rbp - 16], rcx         ; a1
    mov [rbp - 24], r8          ; a2
    mov [rbp - 32], r9          ; a3

    mov r12d, edi               ; fd
    mov r13, rsi                ; fmt
    xor r14d, r14d              ; arg_index = 0

.scan:
    movzx eax, byte [r13]
    test al, al
    jz .done

    cmp al, '%'
    je .specifier

    ; Plain text — find end of non-% run
    mov r15, r13
.run:
    inc r13
    movzx eax, byte [r13]
    test al, al
    jz .flush_run
    cmp al, '%'
    jne .run
.flush_run:
    mov rax, 1
    mov edi, r12d
    mov rsi, r15
    mov rdx, r13
    sub rdx, r15
    syscall
    jmp .scan

.specifier:
    inc r13                     ; skip '%'
    movzx eax, byte [r13]
    inc r13                     ; skip format char

    cmp al, '%'
    je .pct
    cmp al, 's'
    je .str
    cmp al, 'd'
    je .dec
    cmp al, 'u'
    je .dec
    cmp al, 'c'
    je .chr
    cmp al, 'x'
    je .hex
    cmp al, '.'
    je .dot
    jmp .scan                   ; unknown — skip

.pct:
    lea rsi, [r13 - 1]         ; points to the '%' after specifier (actually -1)
    sub rsp, 8
    mov byte [rsp], '%'
    mov rax, 1
    mov edi, r12d
    lea rsi, [rsp]
    mov rdx, 1
    syscall
    add rsp, 8
    jmp .scan

    ; --- Get current arg into rax, increment r14 ---
.get_arg:
    ; r14 = arg_index, args at [rbp-8], [rbp-16], [rbp-24], [rbp-32]
    ; offset = -(r14+1)*8 from rbp
    mov ecx, r14d
    inc ecx
    neg ecx
    movsxd rcx, ecx
    lea rcx, [rcx * 8]
    mov rax, [rbp + rcx]
    inc r14d
    ret

.str:
    call .get_arg               ; rax = string pointer
    test rax, rax
    jz .scan                    ; NULL → skip
    mov rbx, rax
    ; Find length
    xor edx, edx
.str_len:
    cmp byte [rbx + rdx], 0
    je .str_write
    inc edx
    jmp .str_len
.str_write:
    mov rax, 1
    mov edi, r12d
    mov rsi, rbx
    ; edx = len
    syscall
    jmp .scan

.dec:
    call .get_arg               ; rax = value
    mov eax, eax                ; zero-extend 32-bit
    lea rbx, [rel _fmt_numbuf + 20]
    mov byte [rbx], 0
    mov ecx, 10
    test eax, eax
    jnz .dec_loop
    dec rbx
    mov byte [rbx], '0'
    jmp .dec_write
.dec_loop:
    test eax, eax
    jz .dec_write
    xor edx, edx
    div ecx
    add dl, '0'
    dec rbx
    mov [rbx], dl
    jmp .dec_loop
.dec_write:
    lea rdx, [rel _fmt_numbuf + 20]
    sub rdx, rbx
    mov rax, 1
    mov edi, r12d
    mov rsi, rbx
    syscall
    jmp .scan

.chr:
    call .get_arg
    sub rsp, 8
    mov [rsp], al
    mov rax, 1
    mov edi, r12d
    lea rsi, [rsp]
    mov rdx, 1
    syscall
    add rsp, 8
    jmp .scan

.hex:
    call .get_arg
    mov eax, eax
    lea rbx, [rel _fmt_numbuf + 20]
    mov byte [rbx], 0
    test eax, eax
    jnz .hex_loop
    dec rbx
    mov byte [rbx], '0'
    jmp .hex_write
.hex_loop:
    test eax, eax
    jz .hex_write
    mov ecx, eax
    and ecx, 0xF
    cmp ecx, 10
    jl .hex_dig
    add cl, 'a' - 10
    jmp .hex_st
.hex_dig:
    add cl, '0'
.hex_st:
    dec rbx
    mov [rbx], cl
    shr eax, 4
    jmp .hex_loop
.hex_write:
    lea rdx, [rel _fmt_numbuf + 20]
    sub rdx, rbx
    mov rax, 1
    mov edi, r12d
    mov rsi, rbx
    syscall
    jmp .scan

.dot:
    ; Handle %.*s
    movzx eax, byte [r13]
    cmp al, '*'
    jne .scan
    inc r13
    movzx eax, byte [r13]
    cmp al, 's'
    jne .scan
    inc r13
    call .get_arg               ; length
    mov rdx, rax
    call .get_arg               ; string pointer
    test rax, rax
    jz .scan
    mov rsi, rax
    mov rax, 1
    mov edi, r12d
    syscall
    jmp .scan

.done:
    add rsp, 48
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================
; fmt_out(fmt, a0, a1, a2, a3) — stdout
; ============================================================
global fmt_out
fmt_out:
    mov r9, r8
    mov r8, rcx
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov edi, 1
    jmp fmt_fd

; ============================================================
; fmt_err(fmt, a0, a1, a2, a3) — stderr
; ============================================================
global fmt_err
fmt_err:
    mov r9, r8
    mov r8, rcx
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov edi, 2
    jmp fmt_fd

; ============================================================
; fmt_buf — TODO
; ============================================================
global fmt_buf
fmt_buf:
    xor eax, eax
    ret
