# Compiler Internals

How the H++ compiler works, stage by stage.

## Architecture

```
Source (.hpp)
  -> Lexer (text -> tokens)
  -> Macro Preprocessor (expand macros, rewrite [], defx, .)
  -> Parser (tokens -> AST)
  -> Module Resolver (process imports, load .hdef files)
  -> Semantic Analysis (type checking, validation)
  -> Code Generation (AST -> x86-64 NASM assembly)
  -> Backend (nasm -> .o, ld -> executable)
```

## Source Files

```
compiler/
  main.c                    -- entry point, drives pipeline
  common/
    arena.c/h               -- arena (bump) allocator
    strtab.c/h              -- string interning (FNV-1a hash)
    error.c/h               -- error reporting (file:line:col)
    file_io.c/h             -- read source files
    args.c/h                -- CLI argument parsing
    macro.c/h               -- macro preprocessor
    module.c/h              -- module/import resolver
  lexer/
    token.c/h               -- token types and printing
    lexer.c/h               -- character-to-token conversion
  ast/
    ast.c/h                 -- AST node definitions and printer
  parser/
    parser.c/h              -- recursive descent parser
  sema/
    types.c/h               -- type system (HppType, TypeTable)
    symtab.c/h              -- scoped symbol table
    opp_table.c/h           -- operator overload registry
    sema.c/h                -- semantic analysis
  codegen/
    codegen.c/h             -- x86-64 NASM code generator
  backend/
    backend.c/h             -- assembler/linker invocation
  stdlib/
    std_ll.asm              -- 73 syscall wrappers
    std_util.asm            -- 40+ utility functions
    std/                    -- module .hdef and .asm files
```

## Stage 1: Lexer

Converts source text into tokens. Handles:
- Keywords (fn, return, def, if, else, for, while, etc.)
- Identifiers ([a-zA-Z_][a-zA-Z0-9_]*)
- Integer literals (decimal, hex 0x, binary 0b, octal 0o)
- Char literals ('a', '\n')
- String literals ("hello", with escape sequences)
- Operators (52 token types)
- Comments (// and /* */)
- ASM body collection (raw text between asm { })

String interning: all identifier text goes through a hash table so name comparison is pointer equality.

## Stage 2: Macro Preprocessor

Runs on the token array before parsing. Four passes:

1. **Type scanning** -- finds `def TYPE bit[N]` to learn type widths
2. **String var detection** -- tracks variables assigned string literals (for byte array indexing)
3. **Macro/defx extraction** -- removes `macro` and `defx` definitions from token stream, stores them
4. **Array declaration rewrite** -- `int arr[5]` becomes `long arr = int_new(5)`
5. **Bracket rewrite** -- `arr[i]` becomes `int_get(arr, i)` (type-aware)
6. **Struct dot rewrite** -- `p.x` becomes `mem_read32(p, 0)` (offset-aware)
7. **Macro expansion** -- substitute macro calls with body tokens (up to 8 passes for nesting)

## Stage 3: Parser

Recursive descent parser producing an AST with 26 node types.

Expression parsing uses precedence climbing with 12 levels matching C operator precedence.

Key disambiguation:
- `TYPE NAME [` -- array declaration (preprocessed away)
- `TYPE NAME =` -- typed variable declaration
- `NAME(args)` -- function call (vs cast if NAME is a type)
- `* expr` in unary position -- dereference (vs multiply in binary)
- `& NAME` -- address-of (vs bitwise AND in binary)

## Stage 4: Module Resolver

Processes `import` and `link` directives:
1. Builds module path from import segments
2. Searches for .hdef/.asm/.o/.hpp files
3. Parses .hdef for function signatures
4. Registers function signatures in sema
5. Queues .asm/.hpp for compilation, .o for direct linking

## Stage 5: Semantic Analysis

Two passes:
- **Pass 1**: collect all type definitions, function signatures, operator overloads
- **Pass 2**: fully analyze each function body

Checks: type equivalence, bit width matching, const immutability, return types, break/continue in loops, undefined variables, duplicate definitions, literal range fitting.

Integer literal coercion: literals start unresolved, adopt type from context (variable declaration, function argument, binary operation partner).

Implicit widening: smaller types (e.g., int 32-bit) automatically widen to larger types (e.g., long 64-bit) in assignments, function arguments, and expressions.

## Stage 6: Code Generation

Stack-based evaluation strategy:
- All locals on stack (8-byte slots at [rbp - offset])
- Expressions evaluate to rax
- Binary ops: gen left, push, gen right, pop, operate
- Function calls: push args, pop into rdi/rsi/rdx/rcx/r8/r9, call

Operator lowering:

| H++ | x86-64 |
|-----|--------|
| a + b | add eax, ecx |
| a - b | sub eax, ecx |
| a * b | imul eax, ecx |
| a / b | xor edx,edx; div ecx |
| a == b | cmp eax,ecx; sete al |
| &x | lea rax, [rbp-offset] |
| *ptr | mov rax, [rax] |

Control flow: condition, then test al,al, then jz/jnz to labels

String literals: stored in .data section as null-terminated byte sequences

## Stage 7: Backend

1. Write assembly to .asm file
2. Assemble each module: `nasm -f elf64 -o file.o file.asm`
3. Link all .o files: `ld -o output main.o std_ll.o std_util.o [modules...]`
4. Clean up temporary files

No libc -- all OS interaction via raw syscalls through std_ll.
