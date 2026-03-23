import std/{io, fmt, mem};

def int  bit[32];
def long bit[64];
def byte bit[8];

// Define a Point struct: { int x (4 bytes), int y (4 bytes) } = 8 bytes total
defx Point { int x, int y }

// Define a Color struct: { byte r, byte g, byte b } = 3 bytes total
defx Color { byte r, byte g, byte b }

// Define a Rect with nested data
defx Rect { int x, int y, int w, int h }

fn main() -> int {
    // ---- Basic struct ----
    Point p;                    // auto-allocates 8 bytes
    p.x = 10;
    p.y = 20;

    puts("Point: (");
    print_int(p.x);
    puts(", ");
    print_int(p.y);
    println(")");

    // ---- Modify fields ----
    p.x = p.x + 5;
    p.y = p.y * 2;
    puts("Modified: (");
    print_int(p.x);
    puts(", ");
    print_int(p.y);
    println(")");

    // ---- Byte-sized fields ----
    Color c;
    c.r = 255;
    c.g = 128;
    c.b = 0;
    puts("Color: rgb(");
    print_int(c.r);
    puts(", ");
    print_int(c.g);
    puts(", ");
    print_int(c.b);
    println(")");

    // ---- Larger struct ----
    Rect r;
    r.x = 100;
    r.y = 200;
    r.w = 50;
    r.h = 30;
    puts("Rect: x=");
    print_int(r.x);
    puts(" y=");
    print_int(r.y);
    puts(" w=");
    print_int(r.w);
    puts(" h=");
    print_int(r.h);
    print_char('\n');

    // ---- Compute with struct fields ----
    int area = r.w * r.h;
    puts("Area: ");
    print_int(area);
    print_char('\n');

    // ---- Dynamic creation (same as auto, just explicit) ----
    long p2 = alloc(8);     // Point is 8 bytes
    mem_write32(p2, 0, 99);  // p2.x = 99
    mem_write32(p2, 4, 77);  // p2.y = 77  (&x + 4 == &y)
    puts("p2.x=");
    print_int(mem_read32(p2, 0));
    puts(" p2.y=");
    print_int(mem_read32(p2, 4));
    print_char('\n');

    // Cleanup
    free(p, 8);
    free(c, 3);
    free(r, 16);
    free(p2, 8);

    return 0;
}
