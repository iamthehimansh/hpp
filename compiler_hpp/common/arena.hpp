// arena.c rewritten in H++
// Bump allocator using mmap instead of malloc

import std/{sys, mem};

def int  bit[32];
def long bit[64];

// ArenaChunk (32 bytes): base@0, used@8, capacity@16, next@24
const CK_BASE = 0;
const CK_USED = 8;
const CK_CAP  = 16;
const CK_NEXT = 24;
const CK_SIZE = 32;

// Arena (24 bytes): current@0, first@8, default_capacity@16
const AR_CUR    = 0;
const AR_FIRST  = 8;
const AR_DEFCAP = 16;
const AR_SIZE   = 24;

const DEFAULT_ARENA_SIZE = 1048576;
const ALIGNMENT = 8;

fn align_up(n: long, align: long) -> long {
    return (n + align - 1) & (0 - align);
}

fn str_len(s: long) -> long;

fn mmap_alloc(sz: long) -> long {
    // Round up to page size (4096)
    long page_mask = 0 - 4096;
    long size = (sz + 4095) & page_mask;
    if (size == 0) { size = 4096; }
    long ptr = sys_mmap(0, size, 3, 34, -1, 0);
    // Check for mmap error: returns -errno on failure (negative near -1)
    // Success: returns valid positive address
    // Check: if ptr is negative (top bit set), it's an error
    long neg_check = ptr >> 63;
    if (neg_check != 0) {
        return 0;
    }
    return ptr;
}

fn mmap_free(ptr: long, size: long) {
    sys_munmap(ptr, size);
}

fn chunk_create(capacity: long) -> long {
    long chunk = mmap_alloc(CK_SIZE);
    if (chunk == 0) {
        sys_exit(1);
    }
    long base = mmap_alloc(capacity);
    if (base == 0) {
        sys_exit(1);
    }
    mem_write64(chunk, CK_BASE, base);
    mem_write64(chunk, CK_USED, 0);
    mem_write64(chunk, CK_CAP, capacity);
    mem_write64(chunk, CK_NEXT, 0);
    return chunk;
}

fn arena_create(init_cap: long) -> long {
    long initial_capacity = init_cap;
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_ARENA_SIZE;
    }
    long arena = mmap_alloc(AR_SIZE);
    if (arena == 0) {
        sys_exit(1);
    }
    mem_write64(arena, AR_DEFCAP, initial_capacity);
    long first = chunk_create(initial_capacity);
    mem_write64(arena, AR_FIRST, first);
    mem_write64(arena, AR_CUR, first);
    return arena;
}

fn arena_alloc(arena: long, sz: long) -> long {
    long size = align_up(sz, ALIGNMENT);
    long cur = mem_read64(arena, AR_CUR);
    long used = mem_read64(cur, CK_USED);
    long cap = mem_read64(cur, CK_CAP);

    if (used + size > cap) {
        long new_cap = mem_read64(arena, AR_DEFCAP);
        if (size > new_cap) {
            new_cap = size * 2;
        }
        long chunk = chunk_create(new_cap);
        mem_write64(cur, CK_NEXT, chunk);
        mem_write64(arena, AR_CUR, chunk);
        cur = chunk;
        used = 0;
    }

    long base = mem_read64(cur, CK_BASE);
    long ptr = base + used;
    mem_write64(cur, CK_USED, used + size);
    // Zero the allocated memory
    mem_zero(ptr, size);
    return ptr;
}

fn arena_strdup(arena: long, s: long) -> long {
    long len = str_len(s);
    long copy = arena_alloc(arena, len + 1);
    mem_copy(copy, s, len);
    mem_write8(copy, int(len), 0);
    return copy;
}

fn arena_strndup(arena: long, s: long, len: long) -> long {
    long copy = arena_alloc(arena, len + 1);
    mem_copy(copy, s, len);
    mem_write8(copy, int(len), 0);
    return copy;
}

fn arena_destroy(arena: long) {
    long chunk = mem_read64(arena, AR_FIRST);
    while (chunk != 0) {
        long next = mem_read64(chunk, CK_NEXT);
        long base = mem_read64(chunk, CK_BASE);
        long cap = mem_read64(chunk, CK_CAP);
        mmap_free(base, cap);
        mmap_free(chunk, CK_SIZE);
        chunk = next;
    }
    mmap_free(arena, AR_SIZE);
}
