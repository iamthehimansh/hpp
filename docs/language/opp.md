# Operator Overloading (opp)

`opp` defines custom machine code for operators on specific types.

## Syntax

```c
opp type operator type -> result_type {
    asm target {
        ; custom assembly
    }
}
```

## Example

```c
def int bit[32];

opp int + int -> int {
    asm linux {
        add eax, ebx
    }
}
```

## Resolution

1. Compiler checks for an `opp` match: `(left_type, operator, right_type)`
2. If found, uses the custom assembly
3. If not found, uses built-in CPU instruction mapping

## Unary Operators

```c
opp ~int -> int {
    asm linux {
        not eax
    }
}
```

## Use Cases

- Target-specific instruction selection
- Custom big-integer or vector math
- SIMD intrinsics without exposing backend details
- Specializing operations for specific types

## Built-in Defaults

Without any `opp` definitions, the compiler maps operators to standard x86-64 instructions:

| Operator | Instruction |
|----------|-------------|
| `+` | `add` |
| `-` | `sub` |
| `*` | `imul` |
| `/` | `div` |
| `&` | `and` |
| `\|` | `or` |
| `^` | `xor` |
| `~` | `not` |
| `<<` | `shl` |
| `>>` | `shr` |
