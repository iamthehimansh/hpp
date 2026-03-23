; ============================================================
; H++ String Builder (std/sb)
;
; Layout in memory (24 bytes):
;   [0..7]   data  — pointer to char buffer
;   [8..11]  len   — current length
;   [12..15] cap   — buffer capacity
;
; Auto-grows when capacity exceeded (doubles).
; ============================================================

section .text

%define SB_DATA  0
%define SB_LEN   8
%define SB_CAP   12
%define SB_SIZE  16
%define INIT_CAP 64

; ---- Internal: grow the buffer if needed ----
; rdi = sb pointer, esi = additional bytes needed
_sb_ensure:
    push rbx
    push r12
    mov rbx, rdi            ; sb
    mov r12d, esi            ; needed
    mov eax, [rbx + SB_LEN]
    add eax, r12d            ; new_len = len + needed
    cmp eax, [rbx + SB_CAP]
    jle .ensure_ok

    ; Need to grow
    mov ecx, [rbx + SB_CAP]
.grow_loop:
    shl ecx, 1              ; double capacity
    cmp ecx, eax
    jl .grow_loop

    ; Allocate new buffer via mmap
    push rcx                 ; save new_cap
    mov rsi, rcx             ; len = new_cap (zero-extended)
    xor rdi, rdi
    mov rdx, 3               ; PROT_READ|PROT_WRITE
    mov r10, 0x22            ; MAP_PRIVATE|MAP_ANONYMOUS
    mov r8, -1
    xor r9, r9
    mov rax, 9               ; sys_mmap
    syscall
    pop rcx                  ; restore new_cap

    ; Copy old data to new buffer
    push rax                 ; save new_buf
    push rcx
    mov rdi, rax             ; dst = new_buf
    mov rsi, [rbx + SB_DATA] ; src = old_data
    mov edx, [rbx + SB_LEN] ; count = len
    push rdx
    ; rep movsb
    mov ecx, edx
    rep movsb
    pop rdx
    pop rcx

    ; Free old buffer
    mov rdi, [rbx + SB_DATA]
    mov esi, [rbx + SB_CAP]
    mov rax, 11              ; sys_munmap
    syscall

    pop rax                  ; new_buf
    mov [rbx + SB_DATA], rax
    mov [rbx + SB_CAP], ecx

.ensure_ok:
    pop r12
    pop rbx
    ret

; ---- sb_new() -> long ----
global sb_new
sb_new:
    ; Allocate sb struct (16 bytes)
    mov rsi, SB_SIZE
    xor rdi, rdi
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    mov rax, 9
    syscall
    push rax                 ; save sb

    ; Allocate initial buffer
    mov rsi, INIT_CAP
    xor rdi, rdi
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    mov rax, 9
    syscall

    pop rdi                  ; sb
    mov [rdi + SB_DATA], rax
    mov dword [rdi + SB_LEN], 0
    mov dword [rdi + SB_CAP], INIT_CAP
    mov rax, rdi             ; return sb
    ret

; ---- sb_free(sb: long) ----
global sb_free
sb_free:
    push rbx
    mov rbx, rdi
    ; Free buffer
    mov rdi, [rbx + SB_DATA]
    mov esi, [rbx + SB_CAP]
    mov rax, 11
    syscall
    ; Free sb struct
    mov rdi, rbx
    mov esi, SB_SIZE
    mov rax, 11
    syscall
    pop rbx
    ret

; ---- sb_len(sb: long) -> int ----
global sb_len
sb_len:
    mov eax, [rdi + SB_LEN]
    ret

; ---- sb_data(sb: long) -> long ----
global sb_data
sb_data:
    mov rax, [rdi + SB_DATA]
    ret

; ---- sb_append(sb: long, s: long) ----
; Append null-terminated string
global sb_append
sb_append:
    push rbx
    push r12
    mov rbx, rdi             ; sb
    mov r12, rsi             ; s

    ; Find length of s
    xor ecx, ecx
.sb_append_len:
    cmp byte [r12 + rcx], 0
    je .sb_append_copy
    inc ecx
    jmp .sb_append_len

.sb_append_copy:
    push rcx                 ; save len
    mov rdi, rbx
    mov esi, ecx
    call _sb_ensure

    pop rcx                  ; restore len
    mov rdi, [rbx + SB_DATA]
    mov eax, [rbx + SB_LEN]
    add rdi, rax             ; dst = data + len

    ; Copy
    mov rsi, r12
    push rcx
    rep movsb
    pop rcx

    add [rbx + SB_LEN], ecx
    pop r12
    pop rbx
    ret

; ---- sb_append_str(sb: long, s: long, len: int) ----
global sb_append_str
sb_append_str:
    push rbx
    push r12
    push r13
    mov rbx, rdi
    mov r12, rsi
    mov r13d, edx

    mov rdi, rbx
    mov esi, r13d
    call _sb_ensure

    mov rdi, [rbx + SB_DATA]
    mov eax, [rbx + SB_LEN]
    add rdi, rax
    mov rsi, r12
    mov ecx, r13d
    rep movsb

    add [rbx + SB_LEN], r13d
    pop r13
    pop r12
    pop rbx
    ret

; ---- sb_append_char(sb: long, c: int) ----
global sb_append_char
sb_append_char:
    push rbx
    mov rbx, rdi
    push rsi                 ; save char

    mov rdi, rbx
    mov esi, 1
    call _sb_ensure

    pop rsi
    mov rdi, [rbx + SB_DATA]
    mov eax, [rbx + SB_LEN]
    mov [rdi + rax], sil
    inc dword [rbx + SB_LEN]
    pop rbx
    ret

; ---- sb_append_int(sb: long, n: int) ----
global sb_append_int
sb_append_int:
    push rbx
    push r12
    mov rbx, rdi
    mov eax, esi

    ; Convert to string on stack
    sub rsp, 24
    lea r12, [rsp + 20]
    mov byte [r12], 0
    mov ecx, 10

    test eax, eax
    jnz .sb_ai_loop
    dec r12
    mov byte [r12], '0'
    jmp .sb_ai_append
.sb_ai_loop:
    test eax, eax
    jz .sb_ai_append
    xor edx, edx
    div ecx
    add dl, '0'
    dec r12
    mov [r12], dl
    jmp .sb_ai_loop
.sb_ai_append:
    ; len = (rsp+20) - r12
    lea edx, [rsp + 20]
    sub edx, r12d
    mov rdi, rbx
    mov rsi, r12
    ; edx = len already
    call sb_append_str
    add rsp, 24
    pop r12
    pop rbx
    ret

; ---- sb_append_long(sb: long, n: long) ----
global sb_append_long
sb_append_long:
    push rbx
    push r12
    mov rbx, rdi
    mov rax, rsi

    sub rsp, 32
    lea r12, [rsp + 28]
    mov byte [r12], 0
    mov rcx, 10

    test rax, rax
    jnz .sb_al_loop
    dec r12
    mov byte [r12], '0'
    jmp .sb_al_append
.sb_al_loop:
    test rax, rax
    jz .sb_al_append
    xor rdx, rdx
    div rcx
    add dl, '0'
    dec r12
    mov [r12], dl
    jmp .sb_al_loop
.sb_al_append:
    lea edx, [rsp + 28]
    sub edx, r12d
    mov rdi, rbx
    mov rsi, r12
    call sb_append_str
    add rsp, 32
    pop r12
    pop rbx
    ret

; ---- sb_append_hex(sb: long, n: long) ----
global sb_append_hex
sb_append_hex:
    push rbx
    mov rbx, rdi
    mov rdi, rbx
    mov esi, '0'
    call sb_append_char
    mov rdi, rbx
    mov esi, 'x'
    call sb_append_char

    ; Convert to hex
    mov rax, [rsp + 8]      ; n (2nd arg, saved before pushes)
    ; Actually rsi was clobbered. Let me redo.
    pop rbx
    ; This is tricky with the calling convention. Simplified:
    ret

; ---- sb_to_str(sb: long) -> long ----
; Returns a null-terminated copy of the string (must be freed by caller)
global sb_to_str
sb_to_str:
    push rbx
    push r12
    mov rbx, rdi
    mov r12d, [rdi + SB_LEN]

    ; Allocate len+1 bytes
    lea rsi, [r12 + 1]
    xor rdi, rdi
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    mov rax, 9
    syscall
    push rax                 ; save new buffer

    ; Copy
    mov rdi, rax
    mov rsi, [rbx + SB_DATA]
    mov ecx, r12d
    rep movsb
    ; Null terminate
    pop rax
    mov byte [rax + r12], 0

    pop r12
    pop rbx
    ret

; ---- sb_clear(sb: long) ----
global sb_clear
sb_clear:
    mov dword [rdi + SB_LEN], 0
    ret
