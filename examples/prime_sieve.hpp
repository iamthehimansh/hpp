// ============================================================
// Prime Sieve — Sieve of Eratosthenes
// Finds all primes up to N, counts twin primes, computes sum
// Demonstrates: memory management, algorithms, bit manipulation
// ============================================================

import std/{io, fmt, mem, proc};

def int  bit[32];
def long bit[64];
def byte bit[8];

// ---- String helpers (no string literals in v1) ----

fn print_s_primes_up_to() {
    // "Primes up to "
    print_char(80);  print_char(114); print_char(105); print_char(109);
    print_char(101); print_char(115); print_char(32);  print_char(117);
    print_char(112); print_char(32);  print_char(116); print_char(111);
    print_char(32);
}

fn print_s_total_primes() {
    // "Total primes: "
    print_char(84);  print_char(111); print_char(116); print_char(97);
    print_char(108); print_char(32);  print_char(112); print_char(114);
    print_char(105); print_char(109); print_char(101); print_char(115);
    print_char(58);  print_char(32);
}

fn print_s_twin_primes() {
    // "Twin primes:  "
    print_char(84);  print_char(119); print_char(105); print_char(110);
    print_char(32);  print_char(112); print_char(114); print_char(105);
    print_char(109); print_char(101); print_char(115); print_char(58);
    print_char(32);  print_char(32);
}

fn print_s_sum() {
    // "Sum of primes: "
    print_char(83);  print_char(117); print_char(109); print_char(32);
    print_char(111); print_char(102); print_char(32);  print_char(112);
    print_char(114); print_char(105); print_char(109); print_char(101);
    print_char(115); print_char(58);  print_char(32);
}

fn print_s_largest() {
    // "Largest prime: "
    print_char(76);  print_char(97);  print_char(114); print_char(103);
    print_char(101); print_char(115); print_char(116); print_char(32);
    print_char(112); print_char(114); print_char(105); print_char(109);
    print_char(101); print_char(58);  print_char(32);
}

fn print_s_separator() {
    // "--------------------"
    for (int i = 0; i < 30; i = i + 1) {
        print_char(45);
    }
    print_newline();
}

fn print_s_twin_pair() {
    // " ("
    print_char(32); print_char(40);
}

fn print_s_comma() {
    // ", "
    print_char(44); print_char(32);
}

fn print_s_rparen() {
    // ")"
    print_char(41);
}

// ---- Sieve of Eratosthenes ----

fn sieve(is_prime: long, limit: int) {
    // Initialize: set all to 1 (prime)
    mem_set(is_prime, 1, limit);

    // 0 and 1 are not prime
    asm linux {
        mov rdi, [rbp - 8]
        mov byte [rdi], 0
        mov byte [rdi + 1], 0
    }

    // Sieve
    for (int i = 2; i * i < limit; i = i + 1) {
        // Check if is_prime[i]
        int val = read_byte(is_prime, i);
        if (val != 0) {
            // Mark multiples as not prime
            for (int j = i * i; j < limit; j = j + i) {
                write_byte(is_prime, j, 0);
            }
        }
    }
}

// ---- Memory access helpers (byte array operations) ----

fn read_byte(base: long, index: int) -> int {
    int result = 0;
    asm linux {
        mov rdi, [rbp - 8]
        mov esi, [rbp - 16]
        movzx eax, byte [rdi + rsi]
        mov [rbp - 24], eax
    }
    return result;
}

fn write_byte(base: long, index: int, val: int) {
    asm linux {
        mov rdi, [rbp - 8]
        mov esi, [rbp - 16]
        mov eax, [rbp - 24]
        mov [rdi + rsi], al
    }
}

fn read_int(base: long, index: int) -> int {
    int result = 0;
    asm linux {
        mov rdi, [rbp - 8]
        mov esi, [rbp - 16]
        shl esi, 2
        mov eax, [rdi + rsi]
        mov [rbp - 24], eax
    }
    return result;
}

fn write_int(base: long, index: int, val: int) {
    asm linux {
        mov rdi, [rbp - 8]
        mov esi, [rbp - 16]
        shl esi, 2
        mov eax, [rbp - 24]
        mov [rdi + rsi], eax
    }
}

// ---- Collect primes into an int array ----

fn collect_primes(is_prime: long, limit: int, primes: long) -> int {
    int count = 0;
    for (int i = 2; i < limit; i = i + 1) {
        int val = read_byte(is_prime, i);
        if (val != 0) {
            write_int(primes, count, i);
            count = count + 1;
        }
    }
    return count;
}

// ---- Insertion sort on int array ----

fn sort(arr: long, n: int) {
    for (int i = 1; i < n; i = i + 1) {
        int key = read_int(arr, i);
        int j = i - 1;
        for (int done = 0; done == 0;) {
            if (j < 0) {
                done = 1;
            } else {
                int val = read_int(arr, j);
                if (val > key) {
                    write_int(arr, j + 1, val);
                    j = j - 1;
                } else {
                    done = 1;
                }
            }
        }
        write_int(arr, j + 1, key);
    }
}

// ---- Count twin primes and print pairs ----

fn count_twins(primes: long, count: int) -> int {
    int twins = 0;
    for (int i = 0; i < count - 1; i = i + 1) {
        int a = read_int(primes, i);
        int b = read_int(primes, i + 1);
        if (b - a == 2) {
            twins = twins + 1;
        }
    }
    return twins;
}

fn print_twin_pairs(primes: long, count: int) {
    int printed = 0;
    for (int i = 0; i < count - 1; i = i + 1) {
        int a = read_int(primes, i);
        int b = read_int(primes, i + 1);
        if (b - a == 2) {
            if (printed > 0) {
                print_s_comma();
            }
            print_s_twin_pair();
            print_int(a);
            print_s_comma();
            print_int(b);
            print_s_rparen();
            printed = printed + 1;
        }
    }
    print_newline();
}

// ---- Compute sum ----

fn sum_primes(primes: long, count: int) -> int {
    int total = 0;
    for (int i = 0; i < count; i = i + 1) {
        int val = read_int(primes, i);
        total = total + val;
    }
    return total;
}

// ---- Print primes in rows of 10 ----

fn print_primes_grid(primes: long, count: int) {
    for (int i = 0; i < count; i = i + 1) {
        int val = read_int(primes, i);

        // Right-align in 6-char field
        if (val < 10) {
            print_char(32); print_char(32); print_char(32);
            print_char(32); print_char(32);
        } else {
            if (val < 100) {
                print_char(32); print_char(32); print_char(32);
                print_char(32);
            } else {
                if (val < 1000) {
                    print_char(32); print_char(32); print_char(32);
                } else {
                    if (val < 10000) {
                        print_char(32); print_char(32);
                    } else {
                        print_char(32);
                    }
                }
            }
        }
        print_int(val);

        // Newline every 10 primes
        int col = i + 1;
        int rem = col % 10;
        if (rem == 0) {
            print_newline();
        }
    }

    // Final newline if not already printed
    int rem = count % 10;
    if (rem != 0) {
        print_newline();
    }
}

// ---- Main ----

fn main() -> int {
    int limit = 100000;

    // Allocate sieve array (1 byte per number)
    long sieve_arr = alloc(limit);
    if (sieve_arr == 0) {
        return 1;
    }

    // Allocate primes array (4 bytes per prime, max ~170 primes below 1000)
    int primes_bytes = limit * 4;
    long primes = alloc(primes_bytes);
    if (primes == 0) {
        free(sieve_arr, limit);
        return 1;
    }

    // Run sieve
    sieve(sieve_arr, limit);

    // Collect primes
    int count = collect_primes(sieve_arr, limit, primes);

    // Sort (already sorted from sieve, but proves the sort works)
    sort(primes, count);

    // Print header
    print_s_separator();
    print_s_primes_up_to();
    print_int(limit);
    print_newline();
    print_s_separator();

    // Print primes in grid
    print_primes_grid(primes, count);
    print_newline();

    // Statistics
    print_s_separator();

    print_s_total_primes();
    print_int(count);
    print_newline();

    int sum = sum_primes(primes, count);
    print_s_sum();
    print_int(sum);
    print_newline();

    int largest = read_int(primes, count - 1);
    print_s_largest();
    print_int(largest);
    print_newline();

    // Twin primes
    int twins = count_twins(primes, count);
    print_s_twin_primes();
    print_int(twins);
    print_newline();

    // Print twin pairs
    print_twin_pairs(primes, count);

    print_s_separator();

    // Cleanup
    free(primes, primes_bytes);
    free(sieve_arr, limit);

    return 0;
}
