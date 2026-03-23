# H++ Documentation

Welcome to the H++ programming language documentation.

## Quick Links

| Document | Description |
|----------|-------------|
| [Getting Started](getting-started/README.md) | Install, build, and write your first program |
| [Language Reference](language/README.md) | Complete language syntax and features |
| [Standard Library](stdlib/README.md) | All built-in functions and modules |
| [Compiler Internals](compiler/README.md) | How the compiler is implemented |
| [Examples](examples/README.md) | Example programs with explanations |

## What is H++?

H++ is a statically-typed, natively-compiled language built on a single primitive: `bit`. All types are zero-cost abstractions over bit-level representations, compiled directly to x86-64 machine code.

### Key Features

- Single primitive: everything is `bit[n]`
- Native compilation to Linux x86-64 via NASM
- Module system with `import`/`link`
- Macro preprocessor for metaprogramming
- String and char literals with escape sequences
- Array syntax with `[]` for all types
- Struct-like `defx` with dot notation
- Pointer operations: `&var`, `*ptr`
- 73 Linux syscall wrappers + 40+ utility functions
- Inline assembly with `asm linux { ... }`
- Operator overloading with `opp`

### Example

```c
import std/{io, fmt, arr};

def int bit[32];

fn main() -> int {
    int nums[] = [10, 20, 30, 40, 50];

    int sum = 0;
    for (int i = 0; i < 5; i = i + 1) {
        sum = sum + nums[i];
    }

    puts("Sum: ");
    print_int(sum);
    print_char('\n');

    int_free(nums, 5);
    return 0;
}
```
