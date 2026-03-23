#!/bin/bash
cd /mnt/h/H++
HPP=./build/hpp

# Compile all H++ modules
for m in \
    "compiler_hpp/lexer/token.hpp token_hpp" \
    "compiler_hpp/common/file_io.hpp file_io_hpp" \
    "compiler_hpp/common/strtab.hpp strtab_hpp" \
    "compiler_hpp/sema/types.hpp types_hpp" \
    "compiler_hpp/sema/opp_table.hpp opp_table_hpp" \
    "compiler_hpp/sema/symtab.hpp symtab_hpp" \
    "compiler_hpp/common/arena.hpp arena_hpp" \
    "compiler_hpp/common/args.hpp args_hpp" \
    "compiler_hpp/backend/backend.hpp backend_hpp" \
    "compiler_hpp/ast/ast.hpp ast_hpp" \
    "compiler_hpp/lexer/lexer.hpp lexer_hpp" \
    "compiler_hpp/common/module.hpp module_hpp"; do
    src=$(echo $m | cut -d" " -f1); name=$(echo $m | cut -d" " -f2)
    $HPP --lib -S $src -o /tmp/${name}.asm 2>&1
    nasm -f elf64 -o /tmp/${name}.o /tmp/${name}.asm
done

# C modules
for f in compiler/codegen/codegen.c \
         compiler/common/macro.c compiler/main.c \
         compiler/parser/parser.c compiler/sema/sema.c; do
    gcc -Wall -Wextra -std=c11 -g -Icompiler -c -o "/tmp/hpp_sh_$(basename $f .c).o" "$f"
done
rm -f /tmp/hpp_sh_error.o  # remove stale old error.c object
gcc -Wall -Wextra -std=c11 -g -Icompiler -c -o /tmp/error_bridge.o compiler_hpp/common/error_bridge.c
gcc -Wall -Wextra -std=c11 -g -Icompiler -c -o /tmp/types_globals.o compiler_hpp/sema/types_globals.c
nasm -f elf64 -o /tmp/std_ll.o compiler/stdlib/std_ll.asm
nasm -f elf64 -o /tmp/bridge.o /tmp/bridge.asm
nasm -f elf64 -o /tmp/printf.o compiler/stdlib/std/printf.asm

gcc -no-pie -o build/hpp_hybrid \
    /tmp/ast_hpp.o /tmp/backend_hpp.o /tmp/hpp_sh_codegen.o \
    /tmp/arena_hpp.o /tmp/args_hpp.o /tmp/error_bridge.o \
    /tmp/hpp_sh_macro.o /tmp/module_hpp.o \
    /tmp/lexer_hpp.o /tmp/hpp_sh_main.o \
    /tmp/hpp_sh_parser.o /tmp/hpp_sh_sema.o \
    /tmp/token_hpp.o \
    /tmp/file_io_hpp.o /tmp/strtab_hpp.o \
    /tmp/types_hpp.o /tmp/types_globals.o \
    /tmp/opp_table_hpp.o /tmp/symtab_hpp.o \
    /tmp/std_ll.o /tmp/bridge.o /tmp/printf.o \
    -z noexecstack

echo "BUILD OK"

H=./build/hpp_hybrid; PASS=0; FAIL=0; TOTAL=0
t() { TOTAL=$((TOTAL+1)); if [ -n "$3" ]; then cd "$3"; r=$(../../build/hpp_hybrid "$2" -o /tmp/hpp_ht 2>&1); rc=$?; cd /mnt/h/H++; else r=$($H "$2" -o /tmp/hpp_ht 2>&1); rc=$?; fi; if [ $rc -ne 0 ]; then echo "FAIL: $1"; FAIL=$((FAIL+1)); return; fi; /tmp/hpp_ht x >/dev/null 2>&1; rrc=$?; if [ $rrc -gt 128 ]; then echo "FAIL: $1 (crash)"; FAIL=$((FAIL+1)); else echo "PASS: $1"; PASS=$((PASS+1)); fi; }
for f in return42 fibonacci arithmetic bitwise ifelse loops hello; do t $f examples/$f.hpp; done
t stdlib_demo examples/stdlib_demo.hpp; t memory_demo examples/memory_demo.hpp
t file_io_demo examples/file_io_demo.hpp; t sleep_demo examples/sleep_demo.hpp
t strings_demo examples/strings_demo.hpp; t import_demo examples/import_demo.hpp
t import_multi examples/import_multi.hpp; t import_all examples/import_all.hpp
t array_demo examples/array_demo.hpp; t bracket_demo examples/bracket_demo.hpp
t array_syntax examples/array_syntax_demo.hpp; t typed_arrays examples/typed_arrays.hpp
t custom_type examples/custom_type_arr.hpp; t macro_demo examples/macro_demo.hpp
t struct_demo examples/struct_demo.hpp; t addr_demo examples/addr_demo.hpp
t deref_demo examples/deref_demo.hpp; t prime_sieve examples/prime_sieve.hpp
t new_features examples/new_features.hpp; t fnptr_demo examples/fnptr_demo.hpp
t selfhost examples/selfhost_ready.hpp
t mylib main.hpp examples/mylib_demo; t hdef_hpp main.hpp examples/hdef_hpp_demo
echo ""
echo "Results: $PASS/$TOTAL passed, $FAIL failed"
