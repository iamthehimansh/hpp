# Examples

All examples are in the `examples/` directory.

## Basic

| File | Description | Key Features |
|------|-------------|--------------|
| `return42.hpp` | Simplest program | `fn main`, return value |
| `fibonacci.hpp` | Recursive Fibonacci | Functions, recursion, if/else |
| `arithmetic.hpp` | All math operators | +, -, *, /, % |
| `bitwise.hpp` | Bit manipulation | &, \|, ^, ~, <<, >> |
| `ifelse.hpp` | Conditional logic | if/else, comparison |
| `loops.hpp` | Loop constructs | for, while |
| `hello.hpp` | Inline assembly | asm linux { syscall } |

## Standard Library

| File | Description | Key Features |
|------|-------------|--------------|
| `stdlib_demo.hpp` | Print, random, time | print_int, random_int, get_time_sec |
| `memory_demo.hpp` | Memory allocation | alloc, free, mem_set, str_len, itoa |
| `file_io_demo.hpp` | File read/write | file_open_write, file_read, sys_unlink |
| `sleep_demo.hpp` | Timed output | sleep_ms |
| `strings_demo.hpp` | String operations | println, str_len, str_cat, file I/O with strings |
| `array_demo.hpp` | Array library | int_new, int_sort, int_print, int_sum |

## Module System

| File | Description | Key Features |
|------|-------------|--------------|
| `import_demo.hpp` | Selective imports | import std/{fmt, io, mem} |
| `import_multi.hpp` | Multi-import | import std/{fmt, io, mem, time, rand, proc} |
| `import_all.hpp` | Import everything | import std |
| `mylib_demo/` | .hdef + .asm library | User assembly library |
| `hdef_obj_demo/` | .hdef + .o library | Pre-compiled library |
| `hdef_hpp_demo/` | .hdef + .hpp library | H++ source library |

## Advanced

| File | Description | Key Features |
|------|-------------|--------------|
| `prime_sieve.hpp` | Sieve of Eratosthenes | Arrays, algorithms, formatted output |
| `macro_demo.hpp` | Macro system | macro definitions, array access macros |
| `bracket_demo.hpp` | [] array syntax | All bracket operations |
| `typed_arrays.hpp` | All array types | byte[], int[], long[], custom types |
| `array_syntax_demo.hpp` | Declaration forms | int arr[n], int arr[] = [...] |
| `custom_type_arr.hpp` | Custom types + strings | def short, string indexing |
| `struct_demo.hpp` | defx structs | Point, Color, Rect with dot notation |
| `addr_demo.hpp` | Address-of operator | &var, pass-by-reference swap |
| `deref_demo.hpp` | Dereference operator | *ptr read/write, double deref, heap |

## Running an Example

```bash
cd H++
make                                           # build compiler
./build/hpp examples/fibonacci.hpp -o fib      # compile
./fib                                          # run (exit code = 55)
echo $?                                        # show exit code
```
