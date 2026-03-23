// strtab.c rewritten in H++
// String interning via FNV-1a hash table with open addressing

import std/{sys, mem};

def int  bit[32];
def long bit[64];
def byte bit[8];

// External C functions (from arena.c)
fn arena_alloc(arena: long, size: long) -> long;
fn arena_strndup(arena: long, s: long, len: long) -> long;

// StrTabEntry layout (24 bytes): str@0, len@8, hash@16
const STE_STR  = 0;
const STE_LEN  = 8;
const STE_HASH = 16;
const STE_SIZE = 24;

// StrTab layout (24 bytes): entries@0, capacity@8, count@12, arena@16
const ST_ENTRIES  = 0;
const ST_CAPACITY = 8;
const ST_COUNT    = 12;
const ST_ARENA    = 16;
const ST_SIZE     = 24;

const INITIAL_CAPACITY = 256;

fn fnv1a(s: long, len: long) -> int {
    int hash = 2166136261;
    long i = 0;
    while (i < len) {
        int ch = mem_read8(s, int(i));
        hash = hash ^ ch;
        hash = hash * 16777619;
        i++;
    }
    return hash;
}

// Get pointer to entry at index in entries array
fn entry_ptr(entries: long, idx: int) -> long {
    long offset = idx * STE_SIZE;
    return entries + offset;
}

fn strtab_create(arena: long) -> long {
    long tab = arena_alloc(arena, ST_SIZE);
    mem_write32(tab, ST_CAPACITY, INITIAL_CAPACITY);
    mem_write32(tab, ST_COUNT, 0);
    mem_write64(tab, ST_ARENA, arena);
    long entries = arena_alloc(arena, INITIAL_CAPACITY * STE_SIZE);
    mem_write64(tab, ST_ENTRIES, entries);
    return tab;
}

fn strtab_grow(tab: long) {
    int old_cap = mem_read32(tab, ST_CAPACITY);
    long old_entries = mem_read64(tab, ST_ENTRIES);
    long arena = mem_read64(tab, ST_ARENA);

    int new_cap = old_cap * 2;
    long new_entries = arena_alloc(arena, new_cap * STE_SIZE);
    mem_write64(tab, ST_ENTRIES, new_entries);
    mem_write32(tab, ST_CAPACITY, new_cap);
    mem_write32(tab, ST_COUNT, 0);

    // Re-insert all existing entries
    for (int i = 0; i < old_cap; i++) {
        long e = entry_ptr(old_entries, i);
        long str = mem_read64(e, STE_STR);
        if (str != 0) {
            int hash = mem_read32(e, STE_HASH);
            int idx = hash & (new_cap - 1);
            long ne = entry_ptr(new_entries, idx);
            while (mem_read64(ne, STE_STR) != 0) {
                idx = (idx + 1) & (new_cap - 1);
                ne = entry_ptr(new_entries, idx);
            }
            // Copy entry
            mem_write64(ne, STE_STR, str);
            mem_write64(ne, STE_LEN, mem_read64(e, STE_LEN));
            mem_write32(ne, STE_HASH, hash);
            int cnt = mem_read32(tab, ST_COUNT);
            mem_write32(tab, ST_COUNT, cnt + 1);
        }
    }
}

fn strtab_intern(tab: long, s: long, len: long) -> long {
    int count = mem_read32(tab, ST_COUNT);
    int capacity = mem_read32(tab, ST_CAPACITY);

    // Check load factor: count >= capacity * 0.75 → count * 4 >= capacity * 3
    if (count * 4 >= capacity * 3) {
        strtab_grow(tab);
        capacity = mem_read32(tab, ST_CAPACITY);
    }

    int hash = fnv1a(s, len);
    int idx = hash & (capacity - 1);
    long entries = mem_read64(tab, ST_ENTRIES);
    long e = entry_ptr(entries, idx);

    while (mem_read64(e, STE_STR) != 0) {
        int ehash = mem_read32(e, STE_HASH);
        long elen = mem_read64(e, STE_LEN);
        if (ehash == hash) {
            if (elen == len) {
                long estr = mem_read64(e, STE_STR);
                int cmp = mem_cmp(estr, s, len);
                if (cmp == 0) {
                    return estr;
                }
            }
        }
        idx = (idx + 1) & (capacity - 1);
        e = entry_ptr(entries, idx);
    }

    // Not found — intern it
    long arena = mem_read64(tab, ST_ARENA);
    long interned = arena_strndup(arena, s, len);
    mem_write64(e, STE_STR, interned);
    mem_write64(e, STE_LEN, len);
    mem_write32(e, STE_HASH, hash);
    int cnt = mem_read32(tab, ST_COUNT);
    mem_write32(tab, ST_COUNT, cnt + 1);
    return interned;
}
