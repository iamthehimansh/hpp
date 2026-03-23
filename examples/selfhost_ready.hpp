import std/{io, fmt, mem, proc};

def int  bit[32];
def long bit[64];
def byte bit[8];

// Feature 2: enum
enum TokenKind {
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR = 10,
    TOK_SLASH,
    TOK_EOF = 99
}

// Feature 5: top-level const
const MAX_TOKENS = 1024;
const VERSION = 1;

// Feature 4: offsetof with defx
defx Token { int kind, int line, long text }

fn main(argc: int, argv: long) -> int {
    // Feature 3: argv access
    puts("Program: ");
    long prog = arg_str(argv, 0);
    println(prog);

    puts("argc = ");
    print_int(argc);
    print_char('\n');

    if (argc > 1) {
        puts("arg[1] = ");
        long a1 = arg_str(argv, 1);
        println(a1);
    }

    // Feature 1: narrowing cast
    long big = 100000;
    int small = int(big);
    puts("int(100000) = ");
    print_int(small);
    print_char('\n');

    byte tiny = byte(big);
    puts("byte(100000) = ");
    print_int(tiny);    // 100000 % 256 = 160
    print_char('\n');

    // Feature 2: enum values
    puts("TOK_PLUS  = "); print_int(TOK_PLUS);  print_char('\n');
    puts("TOK_MINUS = "); print_int(TOK_MINUS); print_char('\n');
    puts("TOK_STAR  = "); print_int(TOK_STAR);  print_char('\n');
    puts("TOK_SLASH = "); print_int(TOK_SLASH); print_char('\n');
    puts("TOK_EOF   = "); print_int(TOK_EOF);   print_char('\n');

    // Feature 5: top-level const
    puts("MAX_TOKENS = "); print_int(MAX_TOKENS); print_char('\n');
    puts("VERSION    = "); print_int(VERSION);    print_char('\n');

    // Feature 4: offsetof
    puts("offsetof(Token, kind) = "); print_int(offsetof(Token, kind)); print_char('\n');
    puts("offsetof(Token, line) = "); print_int(offsetof(Token, line)); print_char('\n');
    puts("offsetof(Token, text) = "); print_int(offsetof(Token, text)); print_char('\n');

    // Use struct with offsetof
    Token t;
    t.kind = TOK_STAR;
    t.line = 42;
    // Verify offsetof matches dot notation
    int k = mem_read32(t, offsetof(Token, kind));
    puts("t.kind via offsetof = "); print_int(k); print_char('\n');

    // Enum in switch
    switch (t.kind) {
        case 0:
            println("plus");
            break;
        case 10:
            println("star!");
            break;
        default:
            println("other");
            break;
    }

    free(t, sizeof(Token));
    return 0;
}
