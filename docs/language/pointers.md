# Pointers

H++ has explicit pointer operations via `&` (address-of) and `*` (dereference).

## Address-Of

`&var` returns the stack address of a variable as `long`:

```c
int x = 42;
long ptr = &x;       // ptr = stack address of x
print_hex(ptr);       // e.g., 0x7ffc12345678
```

## Dereference (Read)

`*ptr` reads the 64-bit value at the address:

```c
long ptr = &x;
int val = *ptr;       // val = 42 (reads x through pointer)
```

## Dereference (Write)

`*ptr = val` writes to the address:

```c
*ptr = 99;            // x is now 99
```

## Double Dereference

```c
long pp = &ptr;       // pointer to pointer
long val = **pp;      // reads through two levels
```

## Pass by Reference

```c
fn swap(a: long, b: long) {
    long tmp = *a;
    *a = *b;
    *b = tmp;
}

int x = 10;
int y = 20;
swap(&x, &y);
// x = 20, y = 10
```

## Heap Pointers

`alloc` returns a pointer — use `*` to read/write:

```c
long p = alloc(8);
*p = 12345;
println_long(*p);     // 12345
free(p, 8);
```

## Typed Memory Access

For sub-64-bit access at pointer addresses, use `mem_read`/`mem_write`:

```c
long buf = alloc(16);
mem_write32(buf, 0, 42);     // write 32-bit int at offset 0
mem_write32(buf, 4, 99);     // write at offset 4
mem_write8(buf, 8, 65);      // write single byte at offset 8

int val = mem_read32(buf, 0); // read 32-bit int
```
