# Arrays

Arrays are heap-allocated and accessed via `[]` syntax. Requires `import std/arr`.

## Declaration

```c
import std/arr;
def int bit[32];

// Fixed size (zero-initialized)
int nums[10];

// With initializer (size inferred)
int primes[] = [2, 3, 5, 7, 11, 13];

// With size and initializer
int fib[5] = [1, 1, 2, 3, 5];

// Expression as size
int n = 100;
int data[n];
```

## Read and Write

```c
nums[0] = 42;           // write
int x = nums[0];        // read
nums[1] = nums[0] * 2;  // read + write
```

## All Element Types

| Type | Functions Used | Element Size |
|------|---------------|-------------|
| `byte arr[n]` | `byte_new/get/set` | 1 byte |
| `int arr[n]` | `int_new/get/set` | 4 bytes |
| `long arr[n]` | `long_new/get/set` | 8 bytes |

Custom types map automatically by bit width:
```c
def short bit[16];   // 16 ≤ 32 → int backend
short data[5];       // uses int_new/int_get/int_set

def bool bit[8];     // 8 ≤ 8 → byte backend
bool flags[3];       // uses byte_new/byte_get/byte_set
```

## Array Functions

```c
int_sort(arr, count);          // sort ascending
int_reverse(arr, count);       // reverse in place
int_print(arr, count);         // print as [1, 2, 3]
int sum = int_sum(arr, count); // sum all elements
int idx = int_find(arr, count, val); // find value (-1 if not found)
int lo = int_min(arr, count);  // minimum
int hi = int_max(arr, count);  // maximum
int_copy(dst, src, count);     // copy array
int_swap(arr, i, j);           // swap two elements
```

## Memory Management

Arrays must be freed manually:
```c
int nums[5];
// ... use nums ...
int_free(nums, 5);      // must match the declaration count

byte buf[100];
byte_free(buf, 100);
```

## String as Byte Array

String literals are automatically treated as byte arrays:
```c
long s = "Hello";
print_char(s[0]);     // 'H' — uses byte_get
s[0] = 'J';           // byte_set — modifies the copy
```
