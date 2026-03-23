# Modules and Imports

## Import Syntax

```c
import std;                    // all stdlib modules
import std/io;                 // single module
import std/{io, fmt, mem};     // multiple modules
import mylib;                  // user library
```

## Link External Object Files

```c
link "path/to/library.o";
```

## Module Resolution

When you write `import mylib`, the compiler searches for:

1. `mylib.hdef` — type/function declarations (required)
2. `mylib.asm` — assembly implementation
3. `mylib.o` — pre-compiled object file
4. `mylib.hpp` — H++ source implementation

Priority: `.o` > `.asm` > `.hpp`

Search paths (in order):
1. Stdlib directory (next to compiler binary)
2. Current working directory

## Creating a Library

### With Assembly

```
mathlib.hdef:
  def int bit[32];
  fn square(n: int) -> int;

mathlib.asm:
  global square
  square:
      mov eax, edi
      imul eax, edi
      ret
```

### With H++ Source

```
utils.hdef:
  def int bit[32];
  fn power(base: int, exp: int) -> int;

utils.hpp:
  def int bit[32];
  fn power(base: int, exp: int) -> int {
      int result = 1;
      for (int i = 0; i < exp; i = i + 1) {
          result = result * base;
      }
      return result;
  }
```

### Pre-compiled

```
lib.hdef:
  def int bit[32];
  fn fast_func(x: int) -> int;

lib.o:
  (pre-compiled binary — no source needed)
```

## Importing

```c
import mathlib;

fn main() -> int {
    return square(7);    // 49
}
```

## Standard Library Modules

| Module | Description |
|--------|-------------|
| `std/io` | I/O: puts, println, print_char, file_* |
| `std/fmt` | Formatting: print_int, print_hex, itoa |
| `std/mem` | Memory: alloc, free, mem_copy, mem_read/write |
| `std/str` | Strings: str_len, str_eq, str_copy |
| `std/arr` | Arrays: int_new/get/set, sort, print |
| `std/time` | Time: sleep_ms, get_time_sec |
| `std/rand` | Random: random_int, random_bytes |
| `std/proc` | Process: exit, abort |
| `std/sys` | All 73 Linux syscalls |
| `std/net` | Network syscalls |
