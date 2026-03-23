import std/{fmt, io, arr};

def int bit[32];
def long bit[64];

fn main() -> int {
    // ---- Create an int array of 10 elements ----
    int n = 10;
    long a = int_new(n);
    if (a == 0) { return 1; }

    // Fill with values: 50, 30, 80, 10, 90, 20, 70, 40, 60, 100
    int_set(a, 0, 50);
    int_set(a, 1, 30);
    int_set(a, 2, 80);
    int_set(a, 3, 10);
    int_set(a, 4, 90);
    int_set(a, 5, 20);
    int_set(a, 6, 70);
    int_set(a, 7, 40);
    int_set(a, 8, 60);
    int_set(a, 9, 100);

    // Print unsorted
    print_str_literal(85, 110, 115, 111, 114, 116, 101, 100);
    int_print(a, n);

    // Sort
    int_sort(a, n);

    // Print sorted
    print_str_literal(83, 111, 114, 116, 101, 100, 32, 32);
    int_print(a, n);

    // Stats
    int total = int_sum(a, n);
    int lo = int_min(a, n);
    int hi = int_max(a, n);

    // "Sum: "
    print_char(83); print_char(117); print_char(109); print_char(58); print_char(32);
    print_int(total);
    print_newline();

    // "Min: "
    print_char(77); print_char(105); print_char(110); print_char(58); print_char(32);
    print_int(lo);
    print_newline();

    // "Max: "
    print_char(77); print_char(97); print_char(120); print_char(58); print_char(32);
    print_int(hi);
    print_newline();

    // Find value 70
    int idx = int_find(a, n, 70);
    // "Find 70: index "
    print_char(70); print_char(105); print_char(110); print_char(100);
    print_char(32); print_char(55); print_char(48); print_char(58);
    print_char(32);
    print_int(idx);
    print_newline();

    // Reverse
    int_reverse(a, n);
    // "Reversed"
    print_char(82); print_char(101); print_char(118); print_char(101);
    print_char(114); print_char(115); print_char(101); print_char(100);
    int_print(a, n);

    // ---- Copy to new array ----
    long b = int_new(n);
    int_copy(b, a, n);

    // Modify copy
    int_set(b, 0, 999);
    // "Copy:   "
    print_char(67); print_char(111); print_char(112); print_char(121);
    print_char(58); print_char(32); print_char(32); print_char(32);
    int_print(b, n);

    // Original unchanged
    // "Original"
    print_char(79); print_char(114); print_char(105); print_char(103);
    print_char(105); print_char(110); print_char(97); print_char(108);
    int_print(a, n);

    // ---- Byte array demo ----
    int bcount = 5;
    long bytes = byte_new(bcount);
    byte_set(bytes, 0, 72);   // H
    byte_set(bytes, 1, 101);  // e
    byte_set(bytes, 2, 108);  // l
    byte_set(bytes, 3, 108);  // l
    byte_set(bytes, 4, 111);  // o
    // "Bytes:  "
    print_char(66); print_char(121); print_char(116); print_char(101);
    print_char(115); print_char(58); print_char(32); print_char(32);
    print_str(bytes, bcount);
    print_newline();

    // Cleanup
    byte_free(bytes, bcount);
    int_free(b, n);
    int_free(a, n);

    return 0;
}

// Helper: print 8 chars + ": "
fn print_str_literal(c0: int, c1: int, c2: int, c3: int,
                     c4: int, c5: int, c6: int, c7: int) {
    print_char(c0); print_char(c1); print_char(c2); print_char(c3);
    print_char(c4); print_char(c5); print_char(c6); print_char(c7);
}
