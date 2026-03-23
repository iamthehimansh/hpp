// symtab.c rewritten in H++
// Scoped symbol table with hash table per scope

import std/mem;

def int  bit[32];
def long bit[64];

fn arena_alloc(arena: long, size: long) -> long;
fn str_eq(a: long, b: long) -> int;

// Symbol (88 bytes)
const SYM_NAME   = 0;
const SYM_KIND   = 8;
const SYM_TYPE   = 16;
const SYM_LOC_FN = 24;
const SYM_LOC_LN = 32;
const SYM_LOC_CL = 36;
const SYM_MUTABLE = 40;
const SYM_SDEPTH  = 44;
const SYM_STKOFF  = 48;
const SYM_REGIDX  = 52;
const SYM_FI_PTYPES = 56;
const SYM_FI_PCNT   = 64;
const SYM_FI_RTYPE  = 72;
const SYM_IS_EXT    = 80;
const SYM_SIZE      = 88;

// Scope (32 bytes)
const SC_PARENT  = 0;
const SC_DEPTH   = 8;
const SC_SYMS    = 16;
const SC_CAP     = 24;
const SC_COUNT   = 28;
const SC_SIZE    = 32;

// SymTable (16 bytes)
const STB_CURRENT = 0;
const STB_ARENA   = 8;
const STB_SIZE    = 16;

const SCOPE_INIT_CAP = 32;

fn hash_str_nt(s: long) -> int {
    int h = 2166136261;
    int i = 0;
    int ch = mem_read8(s, i);
    while (ch != 0) {
        h = h ^ ch;
        h = h * 16777619;
        i++;
        ch = mem_read8(s, i);
    }
    return h;
}

fn scope_create(arena: long, parent: long, depth: int) -> long {
    long s = arena_alloc(arena, SC_SIZE);
    mem_write64(s, SC_PARENT, parent);
    mem_write32(s, SC_DEPTH, depth);
    mem_write32(s, SC_CAP, SCOPE_INIT_CAP);
    mem_write32(s, SC_COUNT, 0);
    long sym_bytes = SCOPE_INIT_CAP * 8;
    long syms = arena_alloc(arena, sym_bytes);
    mem_zero(syms, sym_bytes);
    mem_write64(s, SC_SYMS, syms);
    return s;
}

fn scope_find_slot(s: long, name: long) -> int {
    int h = hash_str_nt(name);
    int cap = mem_read32(s, SC_CAP);
    int idx = h & (cap - 1);
    long syms = mem_read64(s, SC_SYMS);

    long sym = mem_read64(syms, idx * 8);
    while (sym != 0) {
        long sname = mem_read64(sym, SYM_NAME);
        if (str_eq(sname, name) != 0) {
            return idx;
        }
        idx = (idx + 1) & (cap - 1);
        sym = mem_read64(syms, idx * 8);
    }
    return idx;
}

fn scope_grow(arena: long, s: long) {
    int old_cap = mem_read32(s, SC_CAP);
    long old_syms = mem_read64(s, SC_SYMS);
    int new_cap = old_cap * 2;
    long new_bytes = new_cap * 8;
    long new_syms = arena_alloc(arena, new_bytes);
    mem_zero(new_syms, new_bytes);

    for (int i = 0; i < old_cap; i++) {
        long sym = mem_read64(old_syms, i * 8);
        if (sym != 0) {
            long sname = mem_read64(sym, SYM_NAME);
            int h = hash_str_nt(sname);
            int idx = h & (new_cap - 1);
            while (mem_read64(new_syms, idx * 8) != 0) {
                idx = (idx + 1) & (new_cap - 1);
            }
            mem_write64(new_syms, idx * 8, sym);
        }
    }
    mem_write64(s, SC_SYMS, new_syms);
    mem_write32(s, SC_CAP, new_cap);
}

fn scope_lookup(s: long, name: long) -> long {
    int slot = scope_find_slot(s, name);
    long syms = mem_read64(s, SC_SYMS);
    return mem_read64(syms, slot * 8);
}

fn symtab_create(arena: long) -> long {
    long tab = arena_alloc(arena, STB_SIZE);
    mem_write64(tab, STB_ARENA, arena);
    long global_scope = scope_create(arena, 0, 0);
    mem_write64(tab, STB_CURRENT, global_scope);
    return tab;
}

fn symtab_enter_scope(tab: long) {
    long arena = mem_read64(tab, STB_ARENA);
    long current = mem_read64(tab, STB_CURRENT);
    int depth = mem_read32(current, SC_DEPTH);
    long child = scope_create(arena, current, depth + 1);
    mem_write64(tab, STB_CURRENT, child);
}

fn symtab_exit_scope(tab: long) {
    long current = mem_read64(tab, STB_CURRENT);
    long parent = mem_read64(current, SC_PARENT);
    if (parent != 0) {
        mem_write64(tab, STB_CURRENT, parent);
    }
}

fn symtab_define(tab: long, name: long, kind: int, type: long, loc: long) -> long {
    // Check duplicate in current scope
    long existing = symtab_lookup_local(tab, name);
    if (existing != 0) {
        return 0;
    }

    long arena = mem_read64(tab, STB_ARENA);
    long sym = arena_alloc(arena, SYM_SIZE);
    mem_zero(sym, SYM_SIZE);
    mem_write64(sym, SYM_NAME, name);
    mem_write32(sym, SYM_KIND, kind);
    mem_write64(sym, SYM_TYPE, type);
    // Copy SourceLoc (24 bytes at loc): filename@0, line@8(wait, check)
    // Actually SourceLoc in C is {const char* filename; int line; int col;} = 16 bytes
    // At sym offset 24: filename(8B)@24, line(4B)@32, col(4B)@36
    mem_write64(sym, SYM_LOC_FN, mem_read64(loc, 0));
    mem_write32(sym, SYM_LOC_LN, mem_read32(loc, 8));
    mem_write32(sym, SYM_LOC_CL, mem_read32(loc, 12));
    mem_write32(sym, SYM_MUTABLE, 0);
    long current = mem_read64(tab, STB_CURRENT);
    mem_write32(sym, SYM_SDEPTH, mem_read32(current, SC_DEPTH));
    mem_write32(sym, SYM_REGIDX, -1);

    int slot = scope_find_slot(current, name);
    long syms = mem_read64(current, SC_SYMS);
    mem_write64(syms, slot * 8, sym);
    int cnt = mem_read32(current, SC_COUNT);
    cnt++;
    mem_write32(current, SC_COUNT, cnt);

    int cap = mem_read32(current, SC_CAP);
    if (cnt * 2 > cap) {
        scope_grow(arena, current);
    }

    return sym;
}

fn symtab_lookup(tab: long, name: long) -> long {
    long s = mem_read64(tab, STB_CURRENT);
    while (s != 0) {
        long sym = scope_lookup(s, name);
        if (sym != 0) {
            return sym;
        }
        s = mem_read64(s, SC_PARENT);
    }
    return 0;
}

fn symtab_lookup_local(tab: long, name: long) -> long {
    long current = mem_read64(tab, STB_CURRENT);
    return scope_lookup(current, name);
}

fn symtab_depth(tab: long) -> int {
    long current = mem_read64(tab, STB_CURRENT);
    return mem_read32(current, SC_DEPTH);
}
