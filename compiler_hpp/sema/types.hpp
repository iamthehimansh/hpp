// types.c rewritten in H++
// Type system: HppType, TypeTable, type operations

import std/{sys, mem, str};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn arena_alloc(arena: long, size: long) -> long;
fn str_eq(a: long, b: long) -> int;
fn get_hpp_type_bit() -> long;
fn get_hpp_type_void() -> long;

// HppType layout (16 bytes): name@0, bit_width@8, is_bit_array@12
const HT_NAME = 0;
const HT_BW   = 8;
const HT_IBA  = 12;
const HT_SIZE = 16;

// TypeEntry layout (16 bytes): name@0, type@8
const TE_NAME = 0;
const TE_TYPE = 8;
const TE_SIZE = 16;

// TypeTable layout (24 bytes): arena@0, entries@8, count@16, capacity@20
const TT_ARENA   = 0;
const TT_ENTRIES  = 8;
const TT_COUNT    = 16;
const TT_CAPACITY = 20;
const TT_SIZE     = 24;

const TYPE_TABLE_INIT_CAP = 64;

// Global singletons (in .data section)
// We need HPP_TYPE_BIT and HPP_TYPE_VOID as globals.
// These are C globals — declared extern so linker finds them.
// Actually, we can't have mutable globals in H++. Use C for these.
// For now, declare them as external.

fn hash_str_null_term(s: long) -> int {
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

fn te_ptr(entries: long, idx: int) -> long {
    long off = idx * TE_SIZE;
    return entries + off;
}

fn find_slot(entries: long, capacity: int, name: long) -> int {
    int h = hash_str_null_term(name);
    int idx = h & (capacity - 1);
    long e = te_ptr(entries, idx);
    long ename = mem_read64(e, TE_NAME);
    while (ename != 0) {
        if (str_eq(ename, name) != 0) {
            return idx;
        }
        idx = (idx + 1) & (capacity - 1);
        e = te_ptr(entries, idx);
        ename = mem_read64(e, TE_NAME);
    }
    return idx;
}

fn grow_table(table: long) {
    int old_cap = mem_read32(table, TT_CAPACITY);
    long old_entries = mem_read64(table, TT_ENTRIES);
    long arena = mem_read64(table, TT_ARENA);

    int new_cap = old_cap * 2;
    long new_size = new_cap * TE_SIZE;
    long new_entries = arena_alloc(arena, new_size);
    mem_zero(new_entries, new_size);

    for (int i = 0; i < old_cap; i++) {
        long oe = te_ptr(old_entries, i);
        long oname = mem_read64(oe, TE_NAME);
        if (oname != 0) {
            int slot = find_slot(new_entries, new_cap, oname);
            long ne = te_ptr(new_entries, slot);
            mem_write64(ne, TE_NAME, oname);
            mem_write64(ne, TE_TYPE, mem_read64(oe, TE_TYPE));
        }
    }
    mem_write64(table, TT_ENTRIES, new_entries);
    mem_write32(table, TT_CAPACITY, new_cap);
}

fn type_table_create(arena: long) -> long {
    long t = arena_alloc(arena, TT_SIZE);
    mem_write64(t, TT_ARENA, arena);
    mem_write32(t, TT_CAPACITY, TYPE_TABLE_INIT_CAP);
    mem_write32(t, TT_COUNT, 0);
    long entry_bytes = TYPE_TABLE_INIT_CAP * TE_SIZE;
    long entries = arena_alloc(arena, entry_bytes);
    mem_zero(entries, entry_bytes);
    mem_write64(t, TT_ENTRIES, entries);
    return t;
}

fn type_table_init_builtins(table: long) {
    // Get HPP_TYPE_BIT address (extern C global)
    long bit_type = get_hpp_type_bit();
    long bit_name = mem_read64(bit_type, HT_NAME);

    long entries = mem_read64(table, TT_ENTRIES);
    int cap = mem_read32(table, TT_CAPACITY);
    int slot = find_slot(entries, cap, bit_name);
    long e = te_ptr(entries, slot);
    mem_write64(e, TE_NAME, bit_name);
    mem_write64(e, TE_TYPE, bit_type);
    int cnt = mem_read32(table, TT_COUNT);
    mem_write32(table, TT_COUNT, cnt + 1);
}

fn type_table_define(table: long, name: long, bit_width: int) -> long {
    long entries = mem_read64(table, TT_ENTRIES);
    int cap = mem_read32(table, TT_CAPACITY);
    int slot = find_slot(entries, cap, name);
    long e = te_ptr(entries, slot);

    if (mem_read64(e, TE_NAME) != 0) {
        return 0;
    }

    long arena = mem_read64(table, TT_ARENA);
    long t = arena_alloc(arena, HT_SIZE);
    mem_write64(t, HT_NAME, name);
    mem_write32(t, HT_BW, bit_width);
    mem_write32(t, HT_IBA, 0);

    mem_write64(e, TE_NAME, name);
    mem_write64(e, TE_TYPE, t);
    int cnt = mem_read32(table, TT_COUNT);
    cnt++;
    mem_write32(table, TT_COUNT, cnt);

    if (cnt * 2 > cap) {
        grow_table(table);
    }
    return t;
}

fn type_table_lookup(table: long, name: long) -> long {
    long entries = mem_read64(table, TT_ENTRIES);
    int cap = mem_read32(table, TT_CAPACITY);
    int slot = find_slot(entries, cap, name);
    long e = te_ptr(entries, slot);
    if (mem_read64(e, TE_NAME) == 0) {
        return 0;
    }
    return mem_read64(e, TE_TYPE);
}

fn type_make_bitn(arena: long, n: int) -> long {
    long t = arena_alloc(arena, HT_SIZE);
    mem_write64(t, HT_NAME, 0);
    mem_write32(t, HT_BW, n);
    mem_write32(t, HT_IBA, 1);
    return t;
}

fn type_equivalent(a: long, b: long) -> int {
    if (a == 0) {
        if (b == 0) { return 1; }
        return 0;
    }
    if (b == 0) { return 0; }
    int aw = mem_read32(a, HT_BW);
    int bw = mem_read32(b, HT_BW);
    if (aw == bw) { return 1; }
    return 0;
}

fn type_can_widen(from: long, to: long) -> int {
    if (from == 0) { return 0; }
    if (to == 0) { return 0; }
    int fw = mem_read32(from, HT_BW);
    int tw = mem_read32(to, HT_BW);
    if (fw <= tw) { return 1; }
    return 0;
}

fn type_reg_size(bit_width: int) -> int {
    if (bit_width <= 8) { return 8; }
    if (bit_width <= 16) { return 16; }
    if (bit_width <= 32) { return 32; }
    return 64;
}

fn type_alignment(bit_width: int) -> int {
    if (bit_width <= 8) { return 1; }
    if (bit_width <= 16) { return 2; }
    if (bit_width <= 32) { return 4; }
    return 8;
}
