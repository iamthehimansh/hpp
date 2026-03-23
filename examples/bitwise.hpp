def byte bit[8];
def int bit[32];

fn main() -> int {
    byte a = 0xAB;
    byte swapped = (a << 4) | (a >> 4);
    // 0xAB swapped nibbles = 0xBA = 186
    // But exit code is truncated to 8 bits, so 186
    int result = 0;
    if (swapped == 0xBA) {
        result = 1;
    }
    return result;
}
