; ============================================================
; H++ Standard Array Library (std/arr)
; Provides byte[], int[], and long[] array operations
; ============================================================

section .text

; ============================================================
; BYTE ARRAYS (1 byte per element)
; ============================================================

; byte_new(count: int) -> long
; Allocates count bytes, zeroed
global byte_new
byte_new:
    ; mmap(NULL, count, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0)
    mov rsi, rdi            ; len = count
    xor rdi, rdi            ; addr = NULL
    mov rdx, 3              ; PROT_READ | PROT_WRITE
    mov r10, 0x22           ; MAP_PRIVATE | MAP_ANONYMOUS
    mov r8, -1              ; fd = -1
    xor r9, r9              ; offset = 0
    mov rax, 9              ; sys_mmap
    syscall
    cmp rax, -4096
    ja .byte_new_fail
    ret
.byte_new_fail:
    xor eax, eax
    ret

; byte_free(arr: long, count: int)
global byte_free
byte_free:
    mov rax, 11             ; sys_munmap
    ; rdi = addr, rsi = count (already widened by caller)
    syscall
    ret

; byte_get(arr: long, i: int) -> int
global byte_get
byte_get:
    mov eax, esi            ; i
    movzx eax, byte [rdi + rax]
    ret

; byte_set(arr: long, i: int, val: int)
global byte_set
byte_set:
    mov rax, rsi            ; i
    mov [rdi + rax], dl     ; val (3rd arg low byte)
    ret

; byte_fill(arr: long, count: int, val: int)
global byte_fill
byte_fill:
    push rdi
    push rcx
    mov al, dl              ; val
    mov ecx, esi            ; count
    rep stosb
    pop rcx
    pop rdi
    ret

; byte_copy(dst: long, src: long, count: int)
global byte_copy
byte_copy:
    push rcx
    mov rcx, rdx            ; count
    ; rdi = dst, rsi = src
    rep movsb
    pop rcx
    ret

; ============================================================
; INT ARRAYS (4 bytes per element)
; ============================================================

; int_new(count: int) -> long
; Allocates count * 4 bytes, zeroed
global int_new
int_new:
    mov rsi, rdi
    shl rsi, 2              ; count * 4
    xor rdi, rdi
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    mov rax, 9
    syscall
    cmp rax, -4096
    ja .int_new_fail
    ret
.int_new_fail:
    xor eax, eax
    ret

; int_free(arr: long, count: int)
global int_free
int_free:
    shl rsi, 2              ; count * 4
    mov rax, 11
    syscall
    ret

; int_get(arr: long, i: int) -> int
global int_get
int_get:
    movsxd rax, esi         ; sign-extend i to 64-bit
    mov eax, [rdi + rax * 4]
    ret

; int_set(arr: long, i: int, val: int)
global int_set
int_set:
    movsxd rax, esi
    mov [rdi + rax * 4], edx
    ret

; int_fill(arr: long, count: int, val: int)
global int_fill
int_fill:
    push rcx
    mov ecx, esi            ; count
    mov eax, edx            ; val
    push rdi
.int_fill_loop:
    test ecx, ecx
    jz .int_fill_done
    mov [rdi], eax
    add rdi, 4
    dec ecx
    jmp .int_fill_loop
.int_fill_done:
    pop rdi
    pop rcx
    ret

; int_copy(dst: long, src: long, count: int)
global int_copy
int_copy:
    push rcx
    mov ecx, edx            ; count
    push rdi
.int_copy_loop:
    test ecx, ecx
    jz .int_copy_done
    mov eax, [rsi]
    mov [rdi], eax
    add rdi, 4
    add rsi, 4
    dec ecx
    jmp .int_copy_loop
.int_copy_done:
    pop rdi
    pop rcx
    ret

; int_swap(arr: long, i: int, j: int)
global int_swap
int_swap:
    movsxd rax, esi         ; i
    movsxd rcx, edx         ; j
    mov r8d, [rdi + rax * 4]
    mov r9d, [rdi + rcx * 4]
    mov [rdi + rax * 4], r9d
    mov [rdi + rcx * 4], r8d
    ret

; int_find(arr: long, count: int, val: int) -> int
; Returns index of first occurrence, or -1 if not found
global int_find
int_find:
    mov ecx, esi            ; count
    mov r8d, edx            ; val to find
    xor eax, eax            ; index = 0
.int_find_loop:
    cmp eax, ecx
    jge .int_find_not_found
    cmp [rdi + rax * 4], r8d
    je .int_find_found
    inc eax
    jmp .int_find_loop
.int_find_found:
    ret                     ; eax = index
.int_find_not_found:
    mov eax, -1
    ret

; int_sum(arr: long, count: int) -> int
global int_sum
int_sum:
    xor eax, eax            ; sum = 0
    mov ecx, esi            ; count
    xor r8d, r8d            ; index = 0
.int_sum_loop:
    cmp r8d, ecx
    jge .int_sum_done
    add eax, [rdi + r8 * 4]
    inc r8d
    jmp .int_sum_loop
.int_sum_done:
    ret

; int_min(arr: long, count: int) -> int
; Returns minimum value. Undefined for count=0.
global int_min
int_min:
    mov eax, [rdi]          ; min = arr[0]
    mov ecx, esi
    mov r8d, 1              ; start at index 1
.int_min_loop:
    cmp r8d, ecx
    jge .int_min_done
    mov edx, [rdi + r8 * 4]
    cmp edx, eax
    cmovb eax, edx          ; if arr[i] < min, min = arr[i] (unsigned)
    inc r8d
    jmp .int_min_loop
.int_min_done:
    ret

; int_max(arr: long, count: int) -> int
global int_max
int_max:
    mov eax, [rdi]          ; max = arr[0]
    mov ecx, esi
    mov r8d, 1
.int_max_loop:
    cmp r8d, ecx
    jge .int_max_done
    mov edx, [rdi + r8 * 4]
    cmp edx, eax
    cmova eax, edx          ; if arr[i] > max, max = arr[i] (unsigned)
    inc r8d
    jmp .int_max_loop
.int_max_done:
    ret

; int_reverse(arr: long, count: int)
global int_reverse
int_reverse:
    xor eax, eax            ; left = 0
    mov ecx, esi
    dec ecx                 ; right = count - 1
.int_reverse_loop:
    cmp eax, ecx
    jge .int_reverse_done
    ; swap arr[left] and arr[right]
    mov r8d, [rdi + rax * 4]
    movsxd r9, ecx
    mov r10d, [rdi + r9 * 4]
    mov [rdi + rax * 4], r10d
    mov [rdi + r9 * 4], r8d
    inc eax
    dec ecx
    jmp .int_reverse_loop
.int_reverse_done:
    ret

; int_sort(arr: long, count: int)
; Insertion sort (unsigned comparison)
global int_sort
int_sort:
    push rbx
    push r12
    push r13
    push r14
    mov r12, rdi            ; arr
    mov r13d, esi           ; count
    mov r14d, 1             ; i = 1
.int_sort_outer:
    cmp r14d, r13d
    jge .int_sort_done
    ; key = arr[i]
    movsxd rax, r14d
    mov ebx, [r12 + rax * 4]
    ; j = i - 1
    mov ecx, r14d
    dec ecx
.int_sort_inner:
    cmp ecx, 0
    jl .int_sort_insert
    movsxd rax, ecx
    mov edx, [r12 + rax * 4]
    cmp edx, ebx
    jbe .int_sort_insert     ; if arr[j] <= key, stop (unsigned)
    ; arr[j+1] = arr[j]
    mov r8d, ecx
    inc r8d
    movsxd r9, r8d
    mov [r12 + r9 * 4], edx
    dec ecx
    jmp .int_sort_inner
.int_sort_insert:
    ; arr[j+1] = key
    mov eax, ecx
    inc eax
    movsxd r9, eax
    mov [r12 + r9 * 4], ebx
    inc r14d
    jmp .int_sort_outer
.int_sort_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; int_print(arr: long, count: int)
; Prints array as: [1, 2, 3, 4, 5]
global int_print
int_print:
    push rbx
    push r12
    push r13
    mov r12, rdi            ; arr
    mov r13d, esi           ; count

    ; print '['
    push r12
    push r13
    mov edi, 91
    mov rax, 1
    push rdi
    mov rdi, 1
    lea rsi, [rsp]
    mov rdx, 1
    syscall
    pop rdi
    pop r13
    pop r12

    xor ebx, ebx           ; index = 0
.int_print_loop:
    cmp ebx, r13d
    jge .int_print_end

    ; print comma+space if not first
    test ebx, ebx
    jz .int_print_no_comma
    push rbx
    push r12
    push r13
    ; ", "
    sub rsp, 8
    mov word [rsp], 0x202C  ; ', ' in little-endian
    mov rax, 1
    mov rdi, 1
    lea rsi, [rsp]
    mov rdx, 2
    syscall
    add rsp, 8
    pop r13
    pop r12
    pop rbx
.int_print_no_comma:

    ; get arr[i]
    movsxd rax, ebx
    mov edi, [r12 + rax * 4]

    ; print the number using itoa on stack
    push rbx
    push r12
    push r13
    ; Convert number in edi to string
    ; Use a 24-byte buffer on stack
    sub rsp, 24
    mov eax, edi
    lea r8, [rsp + 20]      ; end of buffer
    mov byte [r8], 0
    mov ecx, 10

    test eax, eax
    jnz .int_print_digits
    dec r8
    mov byte [r8], '0'
    jmp .int_print_write

.int_print_digits:
    test eax, eax
    jz .int_print_write
    xor edx, edx
    div ecx
    add dl, '0'
    dec r8
    mov [r8], dl
    jmp .int_print_digits

.int_print_write:
    ; length = (rsp+20) - r8
    lea rdx, [rsp + 20]
    sub rdx, r8
    mov rax, 1
    mov rdi, 1
    mov rsi, r8
    syscall
    add rsp, 24
    pop r13
    pop r12
    pop rbx

    inc ebx
    jmp .int_print_loop

.int_print_end:
    ; print ']'
    sub rsp, 8
    mov byte [rsp], 93      ; ']'
    mov rax, 1
    mov rdi, 1
    lea rsi, [rsp]
    mov rdx, 1
    syscall
    add rsp, 8

    ; print newline
    sub rsp, 8
    mov byte [rsp], 10
    mov rax, 1
    mov rdi, 1
    lea rsi, [rsp]
    mov rdx, 1
    syscall
    add rsp, 8

    pop r13
    pop r12
    pop rbx
    ret

; ============================================================
; LONG ARRAYS (8 bytes per element)
; ============================================================

; long_new(count: int) -> long
global long_new
long_new:
    mov rsi, rdi
    shl rsi, 3              ; count * 8
    xor rdi, rdi
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    mov rax, 9
    syscall
    cmp rax, -4096
    ja .long_new_fail
    ret
.long_new_fail:
    xor eax, eax
    ret

; long_free(arr: long, count: int)
global long_free
long_free:
    shl rsi, 3              ; count * 8
    mov rax, 11
    syscall
    ret

; long_get(arr: long, i: int) -> long
global long_get
long_get:
    movsxd rax, esi
    mov rax, [rdi + rax * 8]
    ret

; long_set(arr: long, i: int, val: long)
global long_set
long_set:
    movsxd rax, esi
    mov [rdi + rax * 8], rdx
    ret

; long_fill(arr: long, count: int, val: long)
global long_fill
long_fill:
    mov ecx, esi            ; count
    mov rax, rdx            ; val
    push rdi
.long_fill_loop:
    test ecx, ecx
    jz .long_fill_done
    mov [rdi], rax
    add rdi, 8
    dec ecx
    jmp .long_fill_loop
.long_fill_done:
    pop rdi
    ret

; long_copy(dst: long, src: long, count: int)
global long_copy
long_copy:
    mov ecx, edx
    push rdi
.long_copy_loop:
    test ecx, ecx
    jz .long_copy_done
    mov rax, [rsi]
    mov [rdi], rax
    add rdi, 8
    add rsi, 8
    dec ecx
    jmp .long_copy_loop
.long_copy_done:
    pop rdi
    ret

; long_swap(arr: long, i: int, j: int)
global long_swap
long_swap:
    movsxd rax, esi
    movsxd rcx, edx
    mov r8, [rdi + rax * 8]
    mov r9, [rdi + rcx * 8]
    mov [rdi + rax * 8], r9
    mov [rdi + rcx * 8], r8
    ret
