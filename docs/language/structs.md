# Structs (defx)

`defx` defines struct-like types with contiguous C-like memory layout.

## Definition

```c
defx Point { int x, int y }
defx Color { byte r, byte g, byte b }
defx Rect  { int x, int y, int w, int h }
```

## Memory Layout

Fields are laid out contiguously at fixed byte offsets:

```
defx Point { int x, int y }

Offset 0: x (4 bytes)
Offset 4: y (4 bytes)
Total: 8 bytes

&x + 4 == &y   (C-like layout)
```

## Usage

```c
Point p;              // auto-allocates 8 bytes on heap
p.x = 10;            // writes to offset 0
p.y = 20;            // writes to offset 4

int sum = p.x + p.y; // reads from offsets 0 and 4
println("done");

free(p, 8);           // must free manually
```

## Field Types

Field access uses the correct size based on type:

| Field Type | Read Function | Write Function |
|-----------|---------------|----------------|
| `byte` (≤8 bits) | `mem_read8` | `mem_write8` |
| `short` (≤16 bits) | `mem_read16` | `mem_write16` |
| `int` (≤32 bits) | `mem_read32` | `mem_write32` |
| `long` (≤64 bits) | `mem_read64` | `mem_write64` |

## Dynamic Creation

You can also allocate manually:

```c
long p = alloc(8);           // 8 bytes for Point
mem_write32(p, 0, 10);       // p.x = 10
mem_write32(p, 4, 20);       // p.y = 20
free(p, 8);
```

## Passing to Functions

Structs are pointers (`long`), so they're passed by reference:

```c
fn move_point(p: long, dx: int, dy: int) {
    // Access via mem_read/write since function doesn't know it's a Point
    mem_write32(p, 0, mem_read32(p, 0) + dx);
    mem_write32(p, 4, mem_read32(p, 4) + dy);
}

Point p;
p.x = 10;
p.y = 20;
move_point(p, 5, 3);
// p.x = 15, p.y = 23
```

## offsetof

`offsetof(Struct, field)` returns the compile-time byte offset of a field within a struct:

```c
defx Rect { int x, int y, int w, int h }

int off_x = offsetof(Rect, x);   // 0
int off_y = offsetof(Rect, y);   // 4
int off_w = offsetof(Rect, w);   // 8
int off_h = offsetof(Rect, h);   // 12
```

This is useful for manual memory access when working with struct pointers in generic code:

```c
fn read_field(ptr: long, offset: int) -> int {
    return mem_read32(ptr, offset);
}

Rect r;
r.x = 42;
int val = read_field(r, offsetof(Rect, x));   // val = 42
```
