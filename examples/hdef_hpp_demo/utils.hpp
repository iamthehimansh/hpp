def int bit[32];

fn add(a: int, b: int) -> int {
    return a + b;
}

fn multiply(a: int, b: int) -> int {
    return a * b;
}

fn power(base: int, exp: int) -> int {
    int result = 1;
    for (int i = 0; i < exp; i = i + 1) {
        result = result * base;
    }
    return result;
}

fn sum_to(n: int) -> int {
    int total = 0;
    for (int i = 1; i <= n; i = i + 1) {
        total = total + i;
    }
    return total;
}
