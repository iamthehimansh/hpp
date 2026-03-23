# Operators

## Precedence (highest to lowest)

| Level | Operators | Associativity | Description |
|-------|-----------|---------------|-------------|
| 1 | `*ptr` `&var` `!` `~` `-` `++` `--` | Right | Unary (deref, addr, not, bitnot, neg, inc, dec) |
| 2 | `*` `/` `%` | Left | Multiply, divide, modulo |
| 3 | `+` `-` | Left | Add, subtract |
| 4 | `<<` `>>` | Left | Shift left, right |
| 5 | `<` `<=` `>` `>=` | Left | Comparison |
| 6 | `==` `!=` | Left | Equality |
| 7 | `&` | Left | Bitwise AND |
| 8 | `^` | Left | Bitwise XOR |
| 9 | `\|` | Left | Bitwise OR |
| 10 | `&&` | Left | Logical AND (short-circuit) |
| 11 | `\|\|` | Left | Logical OR (short-circuit) |
| 12 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Right | Assignment |

## Arithmetic

```c
a + b     // addition (same bit width required)
a - b     // subtraction
a * b     // multiplication
a / b     // unsigned division
a % b     // unsigned modulo
```

All arithmetic is unsigned in the current version.

## Bitwise

```c
a & b     // AND
a | b     // OR
a ^ b     // XOR
~a        // NOT (complement)
a << n    // shift left
a >> n    // shift right (logical/unsigned)
```

## Comparison

Returns `bit` (1 = true, 0 = false):

```c
a == b    // equal
a != b    // not equal
a < b     // less than (unsigned)
a > b     // greater than
a <= b    // less or equal
a >= b    // greater or equal
```

## Logical

Operands must be `bit`. Short-circuit evaluation:

```c
a && b    // AND — b not evaluated if a is false
a || b    // OR — b not evaluated if a is true
!a        // NOT
```

## Pointer

```c
&x        // address of variable → long
*ptr      // dereference — read value at address
*ptr = v  // dereference — write value to address
```

## Compound Assignment

Compound operators combine an operation with assignment. `a += b` is equivalent to `a = a + b`.

```c
x += 5;       // x = x + 5
x -= 1;       // x = x - 1
x *= 2;       // x = x * 2
x /= 4;       // x = x / 4
x %= 3;       // x = x % 3
x &= 0xFF;    // x = x & 0xFF
x |= mask;    // x = x | mask
x ^= bits;    // x = x ^ bits
x <<= 2;      // x = x << 2
x >>= 1;      // x = x >> 1
```

All ten forms: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`.

## Increment / Decrement

Prefix and postfix forms for `++` and `--`:

```c
int i = 5;
i++;          // post-increment: evaluates to 5, then i becomes 6
++i;          // pre-increment: i becomes 7, evaluates to 7
i--;          // post-decrement: evaluates to 7, then i becomes 6
--i;          // pre-decrement: i becomes 5, evaluates to 5
```

Commonly used in `for` loops:

```c
for (int i = 0; i < 10; i++) {
    print_int(i);
}
```
