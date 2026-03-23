# Strings

## String Literals

String literals are stored in the binary's `.data` section and produce a `long` (pointer):

```c
long msg = "Hello, World!";
println(msg);
```

All strings are null-terminated (`\0` appended automatically).

## Char Literals

Single characters produce their ASCII integer value:

```c
print_char('A');      // prints A (value 65)
print_char('\n');     // newline
int ch = 'Z';        // ch = 90
```

## Escape Sequences

| Escape | Character | Value |
|--------|-----------|-------|
| `\n` | Newline | 10 |
| `\t` | Tab | 9 |
| `\r` | Carriage return | 13 |
| `\0` | Null byte | 0 |
| `\\` | Backslash | 92 |
| `\"` | Double quote | 34 |
| `\'` | Single quote | 39 |

## Print Functions

```c
puts("no newline");        // print string, no newline
println("with newline");   // print string + \n
print_char('X');           // single character
print_str(buf, len);       // print `len` bytes from buffer
eputs("to stderr");        // print to stderr
```

## String Operations

Require `import std/str`:

```c
long s = "Hello";
long len = str_len(s);          // 5
int eq = str_eq(s, "Hello");    // 1 (true)

long buf = alloc(64);
str_copy(buf, s);               // copy to buf
str_cat(buf, ", World!");       // append
println(buf);                   // "Hello, World!"

long pos = str_chr(s, 'l');     // pointer to first 'l'
free(buf, 64);
```

## Indexing Strings

Strings are byte arrays — index with `[]`:

```c
long s = "Hello";
print_char(s[0]);    // 'H'
print_char(s[4]);    // 'o'

// Iterate characters
for (int i = 0; i < 5; i = i + 1) {
    print_char(s[i]);
}
```
