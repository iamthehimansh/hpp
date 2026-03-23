# Variables

## Declaration Forms

| Form | Mutable | Type |
|------|---------|------|
| `const x = 42;` | No | Inferred |
| `let y = 10;` | Yes | Inferred |
| `int z = 100;` | Yes | Explicit |
| `int w;` | Yes | Explicit, zero-initialized |

## Const

```c
const x = 42;      // immutable, type inferred as int (bit[32])
const flag = true;  // type is bit
// x = 10;          // ERROR: cannot assign to const
```

## Let

```c
let y = 10;         // mutable, type inferred
y = 20;             // OK
```

## Typed

```c
int z = 100;        // explicit type
long ptr = 0;       // 64-bit
byte c;             // zero-initialized (c = 0)
```

## Scope

Variables are block-scoped:

```c
{
    int x = 10;
    {
        int x = 20;    // shadows outer x
        print_int(x);  // 20
    }
    print_int(x);      // 10
}
// x not accessible here
```
