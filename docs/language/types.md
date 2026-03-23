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
