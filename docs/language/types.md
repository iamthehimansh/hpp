# Type System

## The Primitive

Everything in H++ is built on one type: `bit`.

```c
bit x = true;     // 1-bit value (0 or 1)
bit[8] raw = 0xFF; // 8 bits
bit[32] word = 0;  // 32 bits
```

## Type Definitions

`def` creates named aliases for bit arrays:

```c
def byte  bit[8];
def short bit[16];
def int   bit[32];
def long  bit[64];
```

These are zero-cost — no runtime overhead. `int` IS `bit[32]`.

## Type Equivalence

Two types are equivalent if they have the same bit width:

```c
def byte  bit[8];
def octet bit[8];

byte x = 10;
octet y = x;      // OK — both are 8 bits
```

## Implicit Widening

Smaller types automatically widen to larger types:

```c
int x = 42;        // 32-bit
long y = x;         // OK — int (32) widens to long (64)
alloc(x);           // OK — int widens to long parameter
```

Narrowing (large → small) is NOT allowed implicitly.

## Type Resolution

All types resolve to `bit[n]` at compile time:
- `byte` → `bit[8]`
- `int` → `bit[32]`
- `long` → `bit[64]`

The compiler tracks bit widths and selects correct register sizes:
| Width | Register | Example |
|-------|----------|---------|
| 8-bit | `al` | `byte` |
| 16-bit | `ax` | `short` |
| 32-bit | `eax` | `int` |
| 64-bit | `rax` | `long` |

## sizeof

`sizeof(type)` returns the compile-time byte size of a type:

```c
int a = sizeof(byte);    // 1
int b = sizeof(short);   // 2
int c = sizeof(int);     // 4
int d = sizeof(long);    // 8
```

Works with any type name, including struct types defined with `defx`.

## null

`null` is a keyword alias for `0`, intended for pointer contexts:

```c
long ptr = null;          // equivalent to: long ptr = 0
if (ptr == null) {
    println("no allocation");
}
```

## Narrowing Cast

Explicit narrowing from a wider type to a narrower type uses function-call syntax with the target type name:

```c
long big = 0x1234ABCD;
int truncated = int(big);     // keeps low 32 bits: 0x1234ABCD
byte lo = byte(big);          // keeps low 8 bits: 0xCD
```

This performs proper truncation (masks to the target bit width). Without an explicit cast, narrowing assignments are a compile-time error.

## Enum

`enum` defines a set of named integer constants:

```c
enum Color { RED, GREEN, BLUE }
// RED = 0, GREEN = 1, BLUE = 2

enum HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
}
```

Values auto-increment from the last assigned value:

```c
enum Example { A, B, C = 5, D }
// A = 0, B = 1, C = 5, D = 6
```

Enum values are compile-time integer constants and can be used anywhere an integer literal is expected.

## Top-Level Constants

`const` declarations outside of functions define compile-time constants:

```c
const MAX = 1024;
const BUFFER_SIZE = 4096;
const NEWLINE = 10;

fn main() -> int {
    long buf = alloc(BUFFER_SIZE);
    // ...
    return 0;
}
```

Top-level constants are replaced with their literal values during preprocessing.

## Char Literals

Single characters in single quotes produce integer values (ASCII code):

```c
byte ch = 'A';           // 65
int newline = '\n';      // 10
```

Supported escape sequences: `\n`, `\t`, `\r`, `\\`, `\'`, `\0`.

## String Literals

Double-quoted strings are stored as null-terminated byte sequences in the `.data` section:

```c
long msg = "hello, world";
println(msg);
```

The variable receives a pointer (`long`) to the first byte of the string. String literals are read-only.
