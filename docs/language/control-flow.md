# Control Flow

## If / Else

Condition must be `bit` (1-bit). Use comparison operators to get `bit` from wider types.

```c
if (x > 0) {
    println("positive");
} else if (x == 0) {
    println("zero");
} else {
    println("negative");
}
```

**Wrong:** `if (x) { ... }` — ERROR if x is not `bit`
**Right:** `if (x != 0) { ... }`

## While

```c
int n = 10;
while (n > 0) {
    print_int(n);
    print_char(' ');
    n = n - 1;
}
// Output: 10 9 8 7 6 5 4 3 2 1
```

## For

```c
for (int i = 0; i < 10; i = i + 1) {
    print_int(i);
}
```

H++ supports `++` and `+=` operators, so the above can also be written as `i++`.

## Break and Continue

```c
for (int i = 0; i < 100; i = i + 1) {
    if (i % 2 == 0) { continue; }   // skip even numbers
    if (i > 20) { break; }           // stop after 20
    print_int(i);
    print_char(' ');
}
// Output: 1 3 5 7 9 11 13 15 17 19
```

## Switch / Case / Default

```c
int code = 2;
switch (code) {
    case 0: {
        println("zero");
        break;
    }
    case 1: {
        println("one");
        break;
    }
    case 2:
    case 3: {
        println("two or three");
        break;
    }
    default: {
        println("other");
        break;
    }
}
```

Each `case` tests against a compile-time integer constant. Execution falls through to the next case unless a `break` is encountered. The `default` branch matches when no `case` matches.

## Return

```c
fn abs(x: int) -> int {
    if (x > 100) { return x; }      // early return
    return 100 - x;
}
```
