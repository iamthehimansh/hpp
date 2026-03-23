; ============================================================
; H++ Standard Utility Library (std_util)
; High-level functions built on Linux syscalls
; All functions use System V AMD64 ABI
; ============================================================

section .data

; Lookup table for hex digits
hex_chars: db "0123456789abcdef"

; Newline character
newline_char: db 10

section .bss

; Scratch buffer for number-to-string conversions (32 bytes)
itoa_buf: resb 32

; Read line buffer (4096 bytes)
readline_buf: resb 4096

section .text

; ============================================================
; OUTPUT - Print functions
; ============================================================

; print_char(c: int) -> void
; Prints a single character (low byte of rdi) to stdout
global print_char
print_char:
    push rdi                ; save char on stack
    mov rax, 1              ; sys_write
    mov rdi, 1              ; stdout
    lea rsi, [rsp]          ; pointer to char on stack
    mov rdx, 1              ; length = 1
    syscall
    pop rdi                 ; clean stack
    ret

; print_newline() -> void
; Prints a newline character to stdout
global print_newline
print_newline:
    mov rax, 1              ; sys_write
    mov rdi, 1              ; stdout
    lea rsi, [rel newline_char]
    mov rdx, 1
    syscall
    ret

; print_str(buf: long, len: long) -> void
; Prints len bytes from buf to stdout
global print_str
print_str:
    mov rax, 1              ; sys_write
    mov rdx, rsi            ; len -> rdx (3rd arg)
    mov rsi, rdi            ; buf -> rsi (2nd arg)
    mov rdi, 1              ; stdout -> rdi (1st arg)
    syscall
    ret

; puts(s: long) -> void
; Prints null-terminated string to stdout
global puts
puts:
    ; First find length
    mov rsi, rdi            ; save s
    xor rdx, rdx            ; len = 0
.puts_len:
    cmp byte [rsi + rdx], 0
    je .puts_write
    inc rdx
    jmp .puts_len
.puts_write:
    mov rax, 1              ; sys_write
    mov rsi, rdi            ; buf = s
    mov rdi, 1              ; fd = stdout
    syscall
    ret

; println(s: long) -> void
; Prints null-terminated string + newline to stdout
global println
println:
    push rbx
    mov rbx, rdi            ; save s
    ; Find length
    xor rdx, rdx
.println_len:
    cmp byte [rbx + rdx], 0
    je .println_write
    inc rdx
    jmp .println_len
.println_write:
    mov rax, 1
    mov rsi, rbx
    mov rdi, 1
    syscall
    ; Print newline
    lea rsi, [rel newline_char]
    mov rax, 1
    mov rdi, 1
    mov rdx, 1
    syscall
    pop rbx
    ret

; eputs(s: long) -> void
; Prints null-terminated string to stderr
global eputs
eputs:
    mov rsi, rdi
    xor rdx, rdx
.eputs_len:
    cmp byte [rsi + rdx], 0
    je .eputs_write
    inc rdx
    jmp .eputs_len
.eputs_write:
    mov rax, 1
    mov rsi, rdi
    mov rdi, 2              ; stderr
    syscall
    ret

; print_int(n: int) -> void
; Prints a 32-bit unsigned integer as decimal to stdout
global print_int
print_int:
    push rbx
    push r12
    mov eax, edi            ; n in eax
    lea r12, [rel itoa_buf + 20] ; end of buffer
    mov byte [r12], 0       ; null terminator
    mov ebx, 10             ; divisor

    ; Handle zero specially
    test eax, eax
    jnz .print_int_loop
    dec r12
    mov byte [r12], '0'
    jmp .print_int_write

.print_int_loop:
    test eax, eax
    jz .print_int_write
    xor edx, edx
    div ebx                 ; eax = eax/10, edx = eax%10
    add dl, '0'             ; convert remainder to ASCII
    dec r12
    mov [r12], dl
    jmp .print_int_loop

.print_int_write:
    ; Calculate length
    lea rdx, [rel itoa_buf + 20]
    sub rdx, r12            ; length = end - start
    ; sys_write(1, r12, rdx)
    mov rax, 1
    mov rdi, 1
    mov rsi, r12
    syscall
    pop r12
    pop rbx
    ret

; print_long(n: long) -> void
; Prints a 64-bit unsigned integer as decimal to stdout
global print_long
print_long:
    push rbx
    push r12
    mov rax, rdi            ; n in rax
    lea r12, [rel itoa_buf + 31] ; end of buffer
    mov byte [r12], 0       ; null terminator
    mov rbx, 10             ; divisor

    test rax, rax
    jnz .print_long_loop
    dec r12
    mov byte [r12], '0'
    jmp .print_long_write

.print_long_loop:
    test rax, rax
    jz .print_long_write
    xor rdx, rdx
    div rbx                 ; rax = rax/10, rdx = rax%10
    add dl, '0'
    dec r12
    mov [r12], dl
    jmp .print_long_loop

.print_long_write:
    lea rdx, [rel itoa_buf + 31]
    sub rdx, r12
    mov rax, 1
    mov rdi, 1
    mov rsi, r12
    syscall
    pop r12
    pop rbx
    ret

; print_signed(n: int) -> void
; Prints a 32-bit signed integer as decimal to stdout
global print_signed
print_signed:
    test edi, edi
    jns .print_signed_pos
    ; Negative: print '-' then negate
    push rdi
    mov edi, '-'
    call print_char
    pop rdi
    neg edi
.print_signed_pos:
    jmp print_int           ; tail call to print_int

; print_hex(n: long) -> void
; Prints a 64-bit value as hexadecimal (with 0x prefix) to stdout
global print_hex
print_hex:
    push rbx
    push r12
    push r13
    mov rbx, rdi            ; value in rbx
    lea r12, [rel itoa_buf + 18]
    mov byte [r12], 0

    ; Handle zero
    test rbx, rbx
    jnz .print_hex_loop
    dec r12
    mov byte [r12], '0'
    jmp .print_hex_prefix

.print_hex_loop:
    test rbx, rbx
    jz .print_hex_prefix
    mov rax, rbx
    and rax, 0x0F           ; low nibble
    lea r13, [rel hex_chars]
    mov al, [r13 + rax]
    dec r12
    mov [r12], al
    shr rbx, 4
    jmp .print_hex_loop

.print_hex_prefix:
    dec r12
    mov byte [r12], 'x'
    dec r12
    mov byte [r12], '0'

    lea rdx, [rel itoa_buf + 18]
    sub rdx, r12
    mov rax, 1
    mov rdi, 1
    mov rsi, r12
    syscall
    pop r13
    pop r12
    pop rbx
    ret

; eprint_str(buf: long, len: long) -> void
; Prints len bytes from buf to stderr
global eprint_str
eprint_str:
    mov rax, 1              ; sys_write
    mov rdx, rsi            ; len
    mov rsi, rdi            ; buf
    mov rdi, 2              ; stderr
    syscall
    ret

; ============================================================
; INPUT - Read functions
; ============================================================

; read_char() -> int
; Reads a single byte from stdin, returns it (or -1 on EOF)
global read_char
read_char:
    sub rsp, 8              ; space for 1 byte (aligned)
    mov rax, 0              ; sys_read
    mov rdi, 0              ; stdin
    lea rsi, [rsp]          ; buffer
    mov rdx, 1              ; read 1 byte
    syscall
    test rax, rax
    jle .read_char_eof
    movzx eax, byte [rsp]  ; return the byte
    add rsp, 8
    ret
.read_char_eof:
    mov eax, -1             ; return -1 on EOF/error
    add rsp, 8
    ret

; read_stdin(buf: long, max_len: long) -> long
; Reads up to max_len bytes from stdin into buf. Returns bytes read.
global read_stdin
read_stdin:
    mov rax, 0              ; sys_read
    mov rdx, rsi            ; max_len -> count
    mov rsi, rdi            ; buf -> buffer
    mov rdi, 0              ; stdin -> fd
    syscall
    ret

; ============================================================
; STRUCT FIELD ACCESS (raw byte-offset reads/writes)
; ============================================================

; mem_read8(ptr: long, offset: int) -> int
global mem_read8
mem_read8:
    movsxd rsi, esi
    movzx eax, byte [rdi + rsi]
    ret

; mem_read16(ptr: long, offset: int) -> int
global mem_read16
mem_read16:
    movsxd rsi, esi
    movzx eax, word [rdi + rsi]
    ret

; mem_read32(ptr: long, offset: int) -> int
global mem_read32
mem_read32:
    movsxd rsi, esi
    mov eax, [rdi + rsi]
    ret

; mem_read64(ptr: long, offset: int) -> long
global mem_read64
mem_read64:
    movsxd rsi, esi
    mov rax, [rdi + rsi]
    ret

; mem_write8(ptr: long, offset: int, val: int)
global mem_write8
mem_write8:
    movsxd rsi, esi
    mov [rdi + rsi], dl
    ret

; mem_write16(ptr: long, offset: int, val: int)
global mem_write16
mem_write16:
    movsxd rsi, esi
    mov [rdi + rsi], dx
    ret

; mem_write32(ptr: long, offset: int, val: int)
global mem_write32
mem_write32:
    movsxd rsi, esi
    mov [rdi + rsi], edx
    ret

; mem_write64(ptr: long, offset: int, val: long)
global mem_write64
mem_write64:
    movsxd rsi, esi
    mov [rdi + rsi], rdx
    ret

; ============================================================
; MEMORY OPERATIONS
; ============================================================

; mem_copy(dst: long, src: long, n: long) -> void
; Copies n bytes from src to dst (no overlap handling)
global mem_copy
mem_copy:
    push rcx
    mov rcx, rdx            ; count
    push rdi
    push rsi
    ; rdi = dst, rsi = src already
    rep movsb
    pop rsi
    pop rdi
    pop rcx
    ret

; mem_set(dst: long, val: int, n: long) -> void
; Sets n bytes at dst to val (low byte)
global mem_set
mem_set:
    push rcx
    push rdi
    mov al, sil             ; val (2nd arg, low byte)
    mov rcx, rdx            ; count
    ; rdi = dst already
    rep stosb
    pop rdi
    pop rcx
    ret

; mem_cmp(a: long, b: long, n: long) -> int
; Compares n bytes. Returns 0 if equal, <0 if a<b, >0 if a>b
global mem_cmp
mem_cmp:
    push rcx
    mov rcx, rdx            ; count
    ; rdi = a, rsi = b
    repe cmpsb
    je .mem_cmp_eq
    movzx eax, byte [rdi-1]
    movzx ecx, byte [rsi-1]
    sub eax, ecx
    pop rcx
    ret
.mem_cmp_eq:
    xor eax, eax
    pop rcx
    ret

; mem_zero(dst: long, n: long) -> void
; Zeros n bytes at dst
global mem_zero
mem_zero:
    push rcx
    push rdi
    xor al, al
    mov rcx, rsi            ; count (2nd arg)
    ; rdi = dst
    rep stosb
    pop rdi
    pop rcx
    ret

; ============================================================
; STRING OPERATIONS (null-terminated)
; ============================================================

; str_len(s: long) -> long
; Returns length of null-terminated string (not counting the null)
global str_len
str_len:
    mov rax, 0              ; counter
    mov rsi, rdi            ; s
.str_len_loop:
    cmp byte [rsi], 0
    je .str_len_done
    inc rax
    inc rsi
    jmp .str_len_loop
.str_len_done:
    ret

; str_eq(a: long, b: long) -> int
; Returns 1 if null-terminated strings are equal, 0 otherwise
global str_eq
str_eq:
    ; rdi = a, rsi = b
.str_eq_loop:
    mov al, [rdi]
    mov cl, [rsi]
    cmp al, cl
    jne .str_eq_no
    test al, al
    jz .str_eq_yes          ; both null = equal
    inc rdi
    inc rsi
    jmp .str_eq_loop
.str_eq_yes:
    mov eax, 1
    ret
.str_eq_no:
    xor eax, eax
    ret

; str_copy(dst: long, src: long) -> long
; Copies null-terminated string from src to dst. Returns dst.
global str_copy
str_copy:
    mov rax, rdi            ; return dst
    ; rdi = dst, rsi = src
.str_copy_loop:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .str_copy_done
    inc rdi
    inc rsi
    jmp .str_copy_loop
.str_copy_done:
    ret

; str_cat(dst: long, src: long) -> long
; Appends src to end of dst (null-terminated). Returns dst.
global str_cat
str_cat:
    mov rax, rdi            ; save dst for return
    ; Find end of dst
.str_cat_find_end:
    cmp byte [rdi], 0
    je .str_cat_copy
    inc rdi
    jmp .str_cat_find_end
.str_cat_copy:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .str_cat_done
    inc rdi
    inc rsi
    jmp .str_cat_copy
.str_cat_done:
    ret

; str_chr(s: long, c: int) -> long
; Finds first occurrence of byte c in string s. Returns pointer or 0 if not found.
global str_chr
str_chr:
    mov al, sil             ; char to find (2nd arg low byte)
.str_chr_loop:
    mov cl, [rdi]
    cmp cl, al
    je .str_chr_found
    test cl, cl
    jz .str_chr_not_found
    inc rdi
    jmp .str_chr_loop
.str_chr_found:
    mov rax, rdi
    ret
.str_chr_not_found:
    xor eax, eax
    ret

; ============================================================
; NUMBER CONVERSION
; ============================================================

; itoa(n: int, buf: long) -> long
; Converts 32-bit unsigned int to decimal string in buf.
; Returns length of string written.
global itoa
itoa:
    push rbx
    push r12
    push r13
    mov eax, edi            ; n
    mov r12, rsi            ; buf
    lea r13, [rel itoa_buf + 20]
    mov byte [r13], 0
    mov ebx, 10

    test eax, eax
    jnz .itoa_loop
    dec r13
    mov byte [r13], '0'
    jmp .itoa_copy

.itoa_loop:
    test eax, eax
    jz .itoa_copy
    xor edx, edx
    div ebx
    add dl, '0'
    dec r13
    mov [r13], dl
    jmp .itoa_loop

.itoa_copy:
    ; Copy from r13 to r12 (user buffer)
    xor rax, rax            ; length counter
.itoa_copy_loop:
    mov cl, [r13 + rax]
    mov [r12 + rax], cl
    test cl, cl
    jz .itoa_done
    inc rax
    jmp .itoa_copy_loop
.itoa_done:
    ; rax = length (not counting null)
    pop r13
    pop r12
    pop rbx
    ret

; atoi(s: long) -> int
; Converts decimal string to 32-bit unsigned int. Stops at first non-digit.
global atoi
atoi:
    xor eax, eax            ; result = 0
    mov ecx, 10             ; multiplier
.atoi_loop:
    movzx edx, byte [rdi]
    sub edx, '0'
    cmp edx, 9
    ja .atoi_done            ; not a digit
    imul eax, ecx            ; result *= 10
    add eax, edx             ; result += digit
    inc rdi
    jmp .atoi_loop
.atoi_done:
    ret

; ============================================================
; MEMORY ALLOCATION (mmap-based)
; ============================================================

; alloc(size: long) -> long
; Allocates size bytes of memory via mmap. Returns pointer or 0 on failure.
; Uses MAP_PRIVATE | MAP_ANONYMOUS (0x22), PROT_READ | PROT_WRITE (0x3)
global alloc
alloc:
    mov rsi, rdi            ; size -> len
    xor rdi, rdi            ; addr = NULL (let kernel choose)
    mov rdx, 3              ; PROT_READ | PROT_WRITE
    mov r10, 0x22           ; MAP_PRIVATE | MAP_ANONYMOUS
    mov r8, -1              ; fd = -1 (no file)
    xor r9, r9              ; offset = 0
    mov rax, 9              ; sys_mmap
    syscall
    ; Check for error (rax > -4096 means error)
    cmp rax, -4096
    ja .alloc_fail
    ret
.alloc_fail:
    xor eax, eax            ; return 0 on failure
    ret

; free(addr: long, size: long) -> int
; Frees memory allocated by alloc. Returns 0 on success.
global free
free:
    mov rax, 11             ; sys_munmap
    ; rdi = addr, rsi = size already
    syscall
    ret

; realloc(old: long, old_size: long, new_size: long) -> long
; Reallocates memory via mremap. Returns new pointer or 0 on failure.
global realloc
realloc:
    mov rax, 25             ; sys_mremap
    ; rdi = old_addr, rsi = old_size, rdx = new_size already
    mov r10, 1              ; MREMAP_MAYMOVE
    syscall
    cmp rax, -4096
    ja .realloc_fail
    ret
.realloc_fail:
    xor eax, eax
    ret

; ============================================================
; FILE HELPERS
; ============================================================

; file_open_read(path: long) -> int
; Opens a file for reading. Returns fd or -1 on error.
global file_open_read
file_open_read:
    mov rax, 2              ; sys_open
    ; rdi = path
    xor rsi, rsi            ; O_RDONLY = 0
    xor rdx, rdx            ; mode = 0
    syscall
    ret

; file_open_write(path: long) -> int
; Opens/creates a file for writing (truncates). Returns fd or -1 on error.
global file_open_write
file_open_write:
    mov rax, 2              ; sys_open
    ; rdi = path
    mov rsi, 0x241          ; O_WRONLY | O_CREAT | O_TRUNC
    mov rdx, 0644o          ; rw-r--r--
    syscall
    ret

; file_open_append(path: long) -> int
; Opens/creates a file for appending. Returns fd or -1 on error.
global file_open_append
file_open_append:
    mov rax, 2              ; sys_open
    ; rdi = path
    mov rsi, 0x441          ; O_WRONLY | O_CREAT | O_APPEND
    mov rdx, 0644o          ; rw-r--r--
    syscall
    ret

; file_read(fd: int, buf: long, count: long) -> long
; Reads from file. Returns bytes read, 0 on EOF, -1 on error.
global file_read
file_read:
    mov rax, 0              ; sys_read
    ; rdi=fd, rsi=buf, rdx=count already in place? No:
    ; rdi=fd, rsi=buf, rdx=count
    ; Actually args are: fd(rdi), buf(rsi), count(rdx) - matches syscall args
    syscall
    ret

; file_write(fd: int, buf: long, count: long) -> long
; Writes to file. Returns bytes written or -1 on error.
global file_write
file_write:
    mov rax, 1              ; sys_write
    syscall
    ret

; file_close(fd: int) -> int
; Closes a file descriptor.
global file_close
file_close:
    mov rax, 3              ; sys_close
    syscall
    ret

; file_size(fd: int) -> long
; Returns the size of an open file (via lseek). Returns -1 on error.
global file_size
file_size:
    push rdi                ; save fd
    ; Seek to end
    mov rax, 8              ; sys_lseek
    ; rdi = fd already
    xor rsi, rsi            ; offset = 0
    mov rdx, 2              ; SEEK_END
    syscall
    push rax                ; save size

    ; Seek back to beginning
    pop rdx                 ; size
    pop rdi                 ; fd
    push rdx                ; save size again
    mov rax, 8              ; sys_lseek
    ; rdi = fd
    xor rsi, rsi            ; offset = 0
    mov rdx, 0              ; SEEK_SET
    syscall

    pop rax                 ; return size
    ret

; ============================================================
; PROCESS HELPERS
; ============================================================

; sleep_ms(ms: int) -> void
; Sleeps for given number of milliseconds
global sleep_ms
sleep_ms:
    sub rsp, 16             ; timespec on stack: [seconds, nanoseconds]
    ; Convert ms to seconds + nanoseconds
    mov eax, edi            ; ms
    xor edx, edx
    mov ecx, 1000
    div ecx                 ; eax = seconds, edx = remaining ms
    mov [rsp], rax          ; tv_sec
    imul rdx, rdx, 1000000 ; remaining ms -> ns
    mov [rsp + 8], rdx      ; tv_nsec

    mov rax, 35             ; sys_nanosleep
    lea rdi, [rsp]          ; req
    xor rsi, rsi            ; rem = NULL
    syscall
    add rsp, 16
    ret

; sleep_sec(sec: int) -> void
; Sleeps for given number of seconds
global sleep_sec
sleep_sec:
    sub rsp, 16
    mov [rsp], rdi           ; tv_sec = seconds
    mov qword [rsp + 8], 0   ; tv_nsec = 0
    mov rax, 35              ; sys_nanosleep
    lea rdi, [rsp]
    xor rsi, rsi
    syscall
    add rsp, 16
    ret

; get_time_sec() -> long
; Returns current unix timestamp in seconds
global get_time_sec
get_time_sec:
    mov rax, 201            ; sys_time
    xor rdi, rdi            ; NULL (return in rax)
    syscall
    ret

; ============================================================
; RANDOM
; ============================================================

; random_bytes(buf: long, count: long) -> long
; Fills buf with count random bytes via getrandom. Returns bytes filled.
global random_bytes
random_bytes:
    mov rax, 318            ; sys_getrandom
    ; rdi = buf, rsi = count already
    xor rdx, rdx            ; flags = 0
    syscall
    ret

; random_int() -> int
; Returns a random 32-bit unsigned integer
global random_int
random_int:
    sub rsp, 8
    mov rax, 318            ; sys_getrandom
    lea rdi, [rsp]          ; buf
    mov rsi, 4              ; 4 bytes
    xor rdx, rdx            ; flags = 0
    syscall
    mov eax, [rsp]          ; return 4 bytes as int
    add rsp, 8
    ret

; ============================================================
; MISC HELPERS
; ============================================================

; exit(code: int) -> noreturn
; Exits the process with given code
global exit
exit:
    mov rax, 60             ; sys_exit
    ; rdi = code
    syscall
    ; never returns

; abort() -> noreturn
; Kills process with SIGABRT
global abort
abort:
    mov rax, 39             ; sys_getpid
    syscall
    mov rdi, rax            ; pid = self
    mov rsi, 6              ; SIGABRT
    mov rax, 62             ; sys_kill
    syscall
    ; fallback exit
    mov rax, 60
    mov rdi, 134            ; 128 + SIGABRT(6)
    syscall

; arg_str(argv: long, index: int) -> long
; Returns pointer to the index-th command line argument string
; argv is a pointer to array of 8-byte pointers (from _start)
global arg_str
arg_str:
    movsxd rax, esi          ; sign-extend index
    mov rax, [rdi + rax * 8] ; load 8-byte pointer from argv array
    ret
