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

### argc / argv

`main` can also accept command-line arguments:

```c
fn main(argc: int, argv: long) -> int {
    // argc = number of arguments (including program name)
    // argv = pointer to array of string pointers
    for (int i = 0; i < argc; i++) {
        long s = arg_str(argv, i);   // get the i-th argument as a string pointer
        println(s);
    }
    return 0;
}
```

`arg_str(argv, index)` returns a `long` pointer to the null-terminated string at the given index in the argv array.

## Recursion

```c
fn fib(n: int) -> int {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
```

## Function Pointers

Take the address of a function with `&` and call it through a `long` variable:

```c
fn add(a: int, b: int) -> int { return a + b; }
fn sub(a: int, b: int) -> int { return a - b; }

fn apply(fp: long, x: int, y: int) -> int {
    return fp(x, y);
}

fn main() -> int {
    long fp = &add;
    int result = fp(3, 4);           // result = 7
    result = apply(&sub, 10, 3);     // result = 7
    return 0;
}
```

Function pointers are stored as `long` values. Calling a `long` variable as a function emits an indirect `call` instruction.
