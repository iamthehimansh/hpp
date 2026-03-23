// opp_table.c rewritten in H++
// Operator overload table: simple array-based storage with linear scan

import std/mem;

def int  bit[32];
def long bit[64];

fn arena_alloc(arena: long, size: long) -> long;
fn type_equivalent(a: long, b: long) -> int;

// OppEntry (48 bytes): left_type@0 right_type@8 result_type@16
//   binop@24 unop@28 is_unary@32 asm_blocks@40
const OE_LEFT   = 0;
const OE_RIGHT  = 8;
const OE_RESULT = 16;
const OE_BINOP  = 24;
const OE_UNOP   = 28;
const OE_ISUN   = 32;
const OE_ASM    = 40;
const OE_SIZE   = 48;

// OppTable (24 bytes): arena@0 entries@8 count@16 capacity@20
const OT_ARENA = 0;
const OT_ENTRIES = 8;
const OT_COUNT = 16;
const OT_CAP = 20;

const OPP_INIT_CAP = 32;

fn oe_ptr(entries: long, idx: int) -> long {
    long off = idx * OE_SIZE;
    return entries + off;
}

fn opp_table_create(arena: long) -> long {
    long t = arena_alloc(arena, 24);
    mem_write64(t, OT_ARENA, arena);
    mem_write32(t, OT_CAP, OPP_INIT_CAP);
    mem_write32(t, OT_COUNT, 0);
    long entries = arena_alloc(arena, OPP_INIT_CAP * OE_SIZE);
    mem_write64(t, OT_ENTRIES, entries);
    return t;
}

fn opp_table_register(table: long, entry: long) {
    int count = mem_read32(table, OT_COUNT);
    int cap = mem_read32(table, OT_CAP);

    if (count >= cap) {
        long arena = mem_read64(table, OT_ARENA);
        int new_cap = cap * 2;
        long new_entries = arena_alloc(arena, new_cap * OE_SIZE);
        long old_entries = mem_read64(table, OT_ENTRIES);
        // Copy old entries
        long copy_bytes = count * OE_SIZE;
        mem_copy(new_entries, old_entries, copy_bytes);
        mem_write64(table, OT_ENTRIES, new_entries);
        mem_write32(table, OT_CAP, new_cap);
    }

    long entries = mem_read64(table, OT_ENTRIES);
    long dst = oe_ptr(entries, count);
    // Copy 48 bytes from entry to dst
    long sz = OE_SIZE;
    mem_copy(dst, entry, sz);
    mem_write32(table, OT_COUNT, count + 1);
}

fn opp_table_lookup_binary(table: long, left: long, op: int, right: long) -> long {
    int count = mem_read32(table, OT_COUNT);
    long entries = mem_read64(table, OT_ENTRIES);

    for (int i = 0; i < count; i++) {
        long e = oe_ptr(entries, i);
        int is_unary = mem_read32(e, OE_ISUN);
        if (is_unary != 0) { continue; }

        int eop = mem_read32(e, OE_BINOP);
        if (eop == op) {
            long el = mem_read64(e, OE_LEFT);
            long er = mem_read64(e, OE_RIGHT);
            if (type_equivalent(el, left) != 0) {
                if (type_equivalent(er, right) != 0) {
                    return e;
                }
            }
        }
    }
    return 0;
}

fn opp_table_lookup_unary(table: long, op: int, operand: long) -> long {
    int count = mem_read32(table, OT_COUNT);
    long entries = mem_read64(table, OT_ENTRIES);

    for (int i = 0; i < count; i++) {
        long e = oe_ptr(entries, i);
        int is_unary = mem_read32(e, OE_ISUN);
        if (is_unary == 0) { continue; }

        int eop = mem_read32(e, OE_UNOP);
        if (eop == op) {
            long el = mem_read64(e, OE_LEFT);
            if (type_equivalent(el, operand) != 0) {
                return e;
            }
        }
    }
    return 0;
}
