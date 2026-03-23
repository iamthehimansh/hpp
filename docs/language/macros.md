# Macros

H++ has a compile-time macro system that performs token-level substitution before parsing.

## Definition

```c
macro name(params) { body }
```

## Simple Macros

```c
macro log(msg) { println(msg) }
macro square(x) { x * x }

log("hello");           // → println("hello")
int s = square(5);      // → int s = 5 * 5
```

## Multi-statement

```c
macro debug(label, val) {
    puts(label);
    puts(": ");
    print_int(val);
    print_char('\n')
}

debug("count", 42);
// Output: count: 42
```

## Array Access Macros

```c
macro get(a, i) { int_get(a, i) }
macro set(a, i, v) { int_set(a, i, v) }

set(arr, 0, 42);
int x = get(arr, 0);
```

Note: The `[]` syntax is actually implemented as a built-in preprocessor rewrite, not a user macro.

## How It Works

1. Macro definitions are extracted from the token stream
2. Macro calls (name followed by `(args)`) are expanded by substituting parameter tokens with argument tokens
3. Expansion runs up to 8 passes to handle nested macros

## Limitations

- No variadic macros (fixed parameter count)
- No `#if` / conditional compilation
- Body must be a valid token sequence (balanced braces)
- Macros are not hygienic — name collisions possible
