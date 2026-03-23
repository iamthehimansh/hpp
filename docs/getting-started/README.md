# Getting Started with H++

## Requirements

- **OS**: Linux x86-64 (or Windows with WSL)
- **Tools**: `gcc`, `nasm`, `make`

### Install on Ubuntu/Debian
```bash
sudo apt install gcc nasm make
```

### Install on WSL (Windows)
```bash
wsl --install -d Ubuntu
wsl -d Ubuntu
sudo apt update && sudo apt install gcc nasm make
```

## Building the Compiler

```bash
git clone https://github.com/iamthehimansh/hpp.git
cd hpp
make
```

This produces `build/hpp` — the H++ compiler.

## Installing Globally

```bash
sudo make install
```

Now `hpp` is available from anywhere:

```bash
hpp --help
hpp hello.hpp -o hello
man hpp
```

Installs to `/usr/local` by default. To change: `sudo make install PREFIX=/opt/hpp`

To uninstall:

```bash
sudo make uninstall
```

## Your First Program

Create `hello.hpp`:
```c
import std/{io, fmt};

def int bit[32];

fn main() -> int {
    println("Hello, World!");
    return 0;
}
```

Compile and run:
```bash
./build/hpp hello.hpp -o hello
./hello
```

Output: `Hello, World!`

## Program Structure

Every H++ program needs:

1. **Imports** (optional but recommended) — bring in stdlib functions
2. **Type definitions** — name your bit widths
3. **A `main` function** — entry point, returns int (exit code)

```c
import std/{io, fmt};          // 1. imports

def int  bit[32];              // 2. type definitions
def long bit[64];
def byte bit[8];

fn main() -> int {             // 3. entry point
    println("Hello!");
    return 0;                  // exit code 0 = success
}
```

## Variables

```c
const x = 42;          // immutable, type inferred (int)
let y = 10;            // mutable, type inferred
int z = 100;           // mutable, explicit type
long ptr = 0;          // 64-bit value
byte c = 65;           // 8-bit value ('A')
```

## Functions

```c
fn add(a: int, b: int) -> int {
    return a + b;
}

fn greet() {                    // no return value
    println("Hi!");
}
```

## Control Flow

```c
// If/else
if (x > 0) {
    println("positive");
} else {
    println("non-positive");
}

// While loop
while (n > 0) {
    n = n - 1;
}

// For loop
for (int i = 0; i < 10; i = i + 1) {
    print_int(i);
    print_char(' ');
}
```

## Arrays

```c
import std/arr;

int nums[5];                         // allocate 5 ints
int primes[] = [2, 3, 5, 7, 11];   // init from values
byte msg[] = ['H', 'i', '!'];       // byte array

nums[0] = 42;                       // write
int x = nums[0];                    // read

int_free(nums, 5);                  // must free manually
```

## Strings

```c
println("Hello, World!");           // string literal
puts("no newline");                 // print without \n
long s = "test";                    // s = pointer to data
int len = str_len(s);               // length = 4
print_char(s[0]);                   // 't' — index as byte array
```

Escape sequences: `\n` `\t` `\r` `\\` `\"` `\0`

## Structs (defx)

```c
defx Point { int x, int y }

Point p;           // auto-allocates 8 bytes
p.x = 10;
p.y = 20;
int sum = p.x + p.y;
free(p, 8);
```

## Pointers

```c
int x = 42;
long ptr = &x;         // address of x
int val = *ptr;         // read through pointer (val = 42)
*ptr = 99;              // write through pointer (x = 99)
```

## Macros

```c
macro log(msg) { println(msg) }
macro max(a, b) { if (a > b) { a } else { b } }

log("debug message");
```

## Compiler Options

```bash
./build/hpp source.hpp [options]

Options:
  -o <output>      Output file name
  -S               Assembly output only
  -t <target>      Target (only "linux" for now)
  --dump-tokens    Print lexer token stream
  --dump-ast       Print parsed AST
  --help           Show help
```

## What's Next?

- [Language Reference](../language/README.md) — full syntax details
- [Standard Library](../stdlib/README.md) — all available functions
- [Examples](../examples/README.md) — more programs to learn from
