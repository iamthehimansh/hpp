def int bit[32];

fn main() -> int {
    int a = 10;
    int b = 3;
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    int rem = a % b;
    // 13 + 7 + 30 + 3 + 1 = 54
    return sum + diff + prod + quot + rem;
}
