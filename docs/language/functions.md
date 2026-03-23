# Functions

## Declaration

```c
fn name(param1: type1, param2: type2) -> return_type {
    // body
    return value;
}
```

## No Return Value

```c
fn greet() {
    println("Hello!");
}
```

## Parameters

Parameters are immutable inside the function:

```c
fn add(a: int, b: int) -> int {
    // a = 10;  // ERROR: parameters are immutable
    return a + b;
}
```

## Forward Declaration

Functions can be declared without a body (for `.hdef` files or external functions):

```c
fn external_func(x: int) -> int;    // declaration only
```

## Calling Convention

H++ uses System V AMD64 ABI:
- Arguments 1-6: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`
- Return value: `rax`
- Caller-saved: `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8-r11`

## Entry Point

Every executable needs `fn main() -> int`. The return value is the process exit code.

```c
fn main() -> int {
    return 0;    // success
}
```

## Recursion

```c
fn fib(n: int) -> int {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
```
