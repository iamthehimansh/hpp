# Operators

## Precedence (highest to lowest)

| Level | Operators | Associativity | Description |
|-------|-----------|---------------|-------------|
| 1 | `*ptr` `&var` `!` `~` `-` | Right | Unary (deref, addr, not, bitnot, neg) |
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
| 12 | `=` | Right | Assignment |

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
