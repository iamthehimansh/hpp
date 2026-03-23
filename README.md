# H++

A statically-typed, natively-compiled programming language built on a single primitive: **`bit`**.

All types are abstractions over bit-level representations. The compiler maps them directly to x86-64 machine instructions with zero overhead.

## Quick Example

```c
import std/{io, fmt, arr};

def int  bit[32];
def long bit[64];

macro get(a, i) { int_get(a, i) }
macro set(a, i, v) { int_set(a, i, v) }

fn main() -> int {
    long nums = int_new(5);
    set(nums, 0, 10);
    set(nums, 1, 20);
    set(nums, 2, 30);

    println("Array:");
    int_print(nums, 3);

    int sum = 0;
    for (int i = 0; i < 3; i = i + 1) {
        sum = sum + get(nums, i);
    }

    puts("Sum: ");
    print_int(sum);
    print_char('\n');

    int_free(nums, 3);
    return 0;
}
```

Output:
```
Array:
[10, 20, 30]
Sum: 60
```

## Features

- **Single primitive** — everything is `bit[n]` at the physical level
- **Zero-cost type aliases** — `def int bit[32];` adds no runtime overhead
- **Native compilation** — compiles to x86-64 Linux executables via NASM
- **Module system** — `import std/io;` or `import std/{io, mem, fmt};`
- **String & char literals** — `"hello"`, `'\n'`, with escape sequences
- **Macro system** — `macro get(a, i) { int_get(a, i) }` for compile-time expansion
- **Inline assembly** — `asm linux { syscall }` for direct hardware access
- **Operator overloading** — `opp int + int -> int { asm linux { add eax, ebx } }`
- **Standard library** — 73 syscall wrappers + 40+ utility functions (I/O, memory, strings, arrays, random, time)
- **External linking** — `link "mylib.o";` to link pre-compiled libraries
- **User libraries** — create `.hdef` + `.asm`/`.hpp`/`.o` modules

## Building

Requires Linux x86-64 (or WSL) with `gcc`, `nasm`, and `make`.

```bash
make
```

## Usage

```bash
# Compile and run
./build/hpp program.hpp -o program
./program

# Assembly output only
./build/hpp program.hpp -S -o program.asm

# Debug: see tokens or AST
./build/hpp program.hpp --dump-tokens
./build/hpp program.hpp --dump-ast
```

## Module System

```c
import std;                    // import all standard library
import std/io;                 // just I/O functions
import std/{io, fmt, mem};     // multiple modules
import mylib;                  // user library (needs mylib.hdef + mylib.asm/.hpp/.o)
link "external.o";             // link pre-compiled object file
```

### Standard Library Modules

| Module | Functions |
|--------|-----------|
| `std/io` | `puts`, `println`, `print_char`, `print_str`, `read_char`, `file_open_read/write`, `file_read`, `file_write`, `file_close` |
| `std/fmt` | `print_int`, `print_long`, `print_hex`, `print_signed`, `itoa`, `atoi` |
| `std/mem` | `alloc`, `free`, `realloc`, `mem_copy`, `mem_set`, `mem_zero`, `mem_cmp` |
| `std/str` | `str_len`, `str_eq`, `str_copy`, `str_cat`, `str_chr` |
| `std/arr` | `int_new/get/set/free`, `int_sort`, `int_print`, `int_find`, `int_sum`, `byte_*`, `long_*` |
| `std/time` | `sleep_ms`, `sleep_sec`, `get_time_sec` |
| `std/rand` | `random_int`, `random_bytes` |
| `std/proc` | `exit`, `abort` |
| `std/sys` | All 73 Linux syscall wrappers (`sys_read`, `sys_write`, `sys_mmap`, ...) |
| `std/net` | `sys_socket`, `sys_bind`, `sys_listen`, `sys_accept`, `sys_connect`, ... |

## Type System

```c
bit                  // 1-bit value
bit[8]               // 8 bits
def byte bit[8];     // named type alias
def int  bit[32];
def long bit[64];
```

Types are equivalent if they have the same bit width. Implicit widening (small → large) is supported.

## Compilation Pipeline

```
Source (.hpp)
  → Lexer (tokens)
  → Macro Expansion
  → Parser (AST)
  → Semantic Analysis (type checking)
  → Code Generation (x86-64 NASM)
  → Assembler (nasm)
  → Linker (ld)
  → Executable
```

## Creating Libraries

**Assembly library:**
```
mylib.hdef    — fn square(n: int) -> int;
mylib.asm     — global square / imul eax, edi / ret
```

**H++ source library:**
```
utils.hdef    — fn power(base: int, exp: int) -> int;
utils.hpp     — fn power(base: int, exp: int) -> int { ... }
```

**Pre-compiled:**
```
lib.hdef      — fn fast_func(x: int) -> int;
lib.o         — pre-compiled object file
```

Import with `import mylib;` — the compiler finds `.hdef` for types and `.asm`/`.hpp`/`.o` for the implementation.

## License

MIT
