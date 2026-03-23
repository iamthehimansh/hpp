def int bit[32];

fn max(a: int, b: int) -> int {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

fn main() -> int {
    return max(17, 42);
}
