# Standard Library Reference

All functions are available via `import std/<module>;`. No import needed per function — importing the module gives you all its functions.

## Modules

- [std/io](#stdio) — Input/Output
- [std/fmt](#stdfmt) — Formatting and number printing
- [std/mem](#stdmem) — Memory allocation and operations
- [std/str](#stdstr) — String operations
- [std/arr](#stdarr) — Array operations
- [std/time](#stdtime) — Time and sleep
- [std/rand](#stdrand) — Random numbers
- [std/sb](#stdsb) — String builder
- [std/proc](#stdproc) — Process control
- [std/sys](#stdsys) — Raw Linux syscalls
- [std/net](#stdnet) — Network syscalls

---

## std/io

`import std/io;`

### Output

| Function | Description |
|----------|-------------|
| `print_char(c: int)` | Print single character (low byte of c) |
| `print_newline()` | Print `\n` |
| `print_str(buf: long, len: long)` | Print `len` bytes from `buf` to stdout |
| `puts(s: long)` | Print null-terminated string to stdout |
| `println(s: long)` | Print null-terminated string + newline |
| `eprint_str(buf: long, len: long)` | Print to stderr |
| `eputs(s: long)` | Print null-terminated string to stderr |

### Input

| Function | Description |
|----------|-------------|
| `read_char() -> int` | Read one byte from stdin (-1 on EOF) |
| `read_stdin(buf: long, max: long) -> long` | Read up to `max` bytes, returns count |

### File Operations

| Function | Description |
|----------|-------------|
| `file_open_read(path: long) -> int` | Open file for reading (returns fd) |
| `file_open_write(path: long) -> int` | Open/create file for writing (truncates) |
| `file_open_append(path: long) -> int` | Open/create file for appending |
| `file_read(fd: int, buf: long, count: long) -> long` | Read from fd |
| `file_write(fd: int, buf: long, count: long) -> long` | Write to fd |
| `file_close(fd: int) -> int` | Close file descriptor |
| `file_size(fd: int) -> long` | Get file size in bytes |

---

## std/fmt

`import std/fmt;`

| Function | Description |
|----------|-------------|
| `print_int(n: int)` | Print unsigned 32-bit decimal |
| `print_long(n: long)` | Print unsigned 64-bit decimal |
| `print_signed(n: int)` | Print signed 32-bit decimal |
| `print_hex(n: long)` | Print as hex with `0x` prefix |
| `itoa(n: int, buf: long) -> long` | Convert int to string in buffer, returns length |
| `atoi(s: long) -> int` | Convert decimal string to int |

---

## std/mem

`import std/mem;`

### Allocation

| Function | Description |
|----------|-------------|
| `alloc(size: long) -> long` | Allocate `size` bytes (0 on failure) |
| `free(addr: long, size: long) -> int` | Free allocation (must match size) |
| `realloc(old: long, old_size: long, new_size: long) -> long` | Resize allocation |

### Operations

| Function | Description |
|----------|-------------|
| `mem_copy(dst: long, src: long, n: long)` | Copy `n` bytes |
| `mem_set(dst: long, val: int, n: long)` | Fill `n` bytes with `val` |
| `mem_cmp(a: long, b: long, n: long) -> int` | Compare (0=equal, <0 or >0) |
| `mem_zero(dst: long, n: long)` | Zero `n` bytes |

### Typed Access (for structs/raw memory)

| Function | Description |
|----------|-------------|
| `mem_read8(ptr: long, offset: int) -> int` | Read byte at ptr+offset |
| `mem_read16(ptr: long, offset: int) -> int` | Read 16-bit at ptr+offset |
| `mem_read32(ptr: long, offset: int) -> int` | Read 32-bit at ptr+offset |
| `mem_read64(ptr: long, offset: int) -> long` | Read 64-bit at ptr+offset |
| `mem_write8(ptr: long, offset: int, val: int)` | Write byte |
| `mem_write16(ptr: long, offset: int, val: int)` | Write 16-bit |
| `mem_write32(ptr: long, offset: int, val: int)` | Write 32-bit |
| `mem_write64(ptr: long, offset: int, val: long)` | Write 64-bit |

---

## std/str

`import std/str;`

All strings are null-terminated.

| Function | Description |
|----------|-------------|
| `str_len(s: long) -> long` | Length (not counting null) |
| `str_eq(a: long, b: long) -> int` | 1 if equal, 0 if not |
| `str_copy(dst: long, src: long) -> long` | Copy string, returns dst |
| `str_cat(dst: long, src: long) -> long` | Append src to dst, returns dst |
| `str_chr(s: long, c: int) -> long` | Find char, returns pointer or 0 |

---

## std/arr

`import std/arr;`

### Byte Arrays

| Function | Description |
|----------|-------------|
| `byte_new(count: int) -> long` | Allocate byte array |
| `byte_free(arr: long, count: int)` | Free byte array |
| `byte_get(arr: long, i: int) -> int` | Read arr[i] |
| `byte_set(arr: long, i: int, val: int)` | Write arr[i] = val |
| `byte_fill(arr: long, count: int, val: int)` | Fill all with val |
| `byte_copy(dst: long, src: long, count: int)` | Copy array |

### Int Arrays

| Function | Description |
|----------|-------------|
| `int_new(count: int) -> long` | Allocate int array |
| `int_free(arr: long, count: int)` | Free int array |
| `int_get(arr: long, i: int) -> int` | Read arr[i] |
| `int_set(arr: long, i: int, val: int)` | Write arr[i] = val |
| `int_fill(arr: long, count: int, val: int)` | Fill all |
| `int_copy(dst: long, src: long, count: int)` | Copy |
| `int_swap(arr: long, i: int, j: int)` | Swap elements |
| `int_find(arr: long, count: int, val: int) -> int` | Find (-1 if not found) |
| `int_sum(arr: long, count: int) -> int` | Sum all elements |
| `int_min(arr: long, count: int) -> int` | Minimum value |
| `int_max(arr: long, count: int) -> int` | Maximum value |
| `int_reverse(arr: long, count: int)` | Reverse in place |
| `int_sort(arr: long, count: int)` | Sort ascending |
| `int_print(arr: long, count: int)` | Print as [1, 2, 3] |

### Long Arrays

| Function | Description |
|----------|-------------|
| `long_new(count: int) -> long` | Allocate long array |
| `long_free(arr: long, count: int)` | Free |
| `long_get(arr: long, i: int) -> long` | Read |
| `long_set(arr: long, i: int, val: long)` | Write |
| `long_fill(arr: long, count: int, val: long)` | Fill |
| `long_copy(dst: long, src: long, count: int)` | Copy |
| `long_swap(arr: long, i: int, j: int)` | Swap |

---

## std/time

`import std/time;`

| Function | Description |
|----------|-------------|
| `sleep_ms(ms: int)` | Sleep for milliseconds |
| `sleep_sec(sec: int)` | Sleep for seconds |
| `get_time_sec() -> long` | Unix timestamp in seconds |

---

## std/rand

`import std/rand;`

| Function | Description |
|----------|-------------|
| `random_int() -> int` | Random 32-bit unsigned integer |
| `random_bytes(buf: long, count: long) -> long` | Fill buffer with random bytes |

---

## std/sb

`import std/sb;`

Dynamic string builder for constructing strings incrementally.

| Function | Description |
|----------|-------------|
| `sb_new() -> long` | Create a new string builder (returns handle) |
| `sb_append(sb: long, s: long)` | Append a null-terminated string |
| `sb_append_char(sb: long, c: int)` | Append a single character |
| `sb_append_int(sb: long, n: int)` | Append an integer as decimal text |
| `sb_append_long(sb: long, n: long)` | Append a long as decimal text |
| `sb_to_str(sb: long) -> long` | Get the built string as a null-terminated pointer |
| `sb_clear(sb: long)` | Reset the builder (keeps allocated memory) |
| `sb_free(sb: long)` | Free all memory used by the builder |

### Example

```c
import std/sb;
import std/io;

fn main() -> int {
    long sb = sb_new();
    sb_append(sb, "Hello, ");
    sb_append(sb, "world");
    sb_append_char(sb, '!');
    println(sb_to_str(sb));    // "Hello, world!"
    sb_free(sb);
    return 0;
}
```

---

## std/proc

`import std/proc;`

| Function | Description |
|----------|-------------|
| `exit(code: int)` | Exit process (no return) |
| `abort()` | Kill with SIGABRT (no return) |
| `arg_str(argv: long, index: int) -> long` | Get pointer to argv string at index |

---

## std/sys

`import std/sys;`

73 raw Linux x86-64 syscall wrappers. All named `sys_<name>`. Return values follow Linux convention (negative = error).

### File I/O
sys_read, sys_write, sys_open, sys_close, sys_stat, sys_fstat, sys_lstat, sys_lseek, sys_pread64, sys_pwrite64, sys_access, sys_pipe, sys_dup, sys_dup2, sys_fcntl, sys_fsync, sys_truncate, sys_ftruncate, sys_rename, sys_unlink, sys_readlink, sys_chmod, sys_fchmod, sys_chown

### Memory
sys_mmap, sys_mprotect, sys_munmap, sys_brk, sys_mremap

### Process
sys_getpid, sys_fork, sys_execve, sys_exit, sys_wait4, sys_kill, sys_getuid, sys_getgid, sys_setuid, sys_setgid, sys_geteuid, sys_getegid, sys_getppid, sys_setsid, sys_exit_group

### Directory
sys_getcwd, sys_chdir, sys_mkdir, sys_rmdir, sys_getdents64

### Network
sys_socket, sys_bind, sys_listen, sys_accept, sys_connect, sys_sendto, sys_recvfrom, sys_shutdown, sys_setsockopt, sys_getsockopt

### Time
sys_nanosleep, sys_gettimeofday, sys_time, sys_clock_gettime

### Signal
sys_rt_sigaction, sys_rt_sigprocmask, sys_pause, sys_alarm

### Misc
sys_poll, sys_ioctl, sys_select, sys_uname, sys_getrusage, sys_sysinfo, sys_getrandom

---

## std/net

`import std/net;`

Convenience import — same functions as the network section of `std/sys`.
