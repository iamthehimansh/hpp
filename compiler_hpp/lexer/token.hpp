// token.c fully rewritten in H++: token_kind_str + token_print

import std/{io, fmt, printf, mem};

def int  bit[32];
def long bit[64];
def byte bit[8];

// Token offsets (56 bytes)
const TK_KIND      = 0;
const TK_LOC_LINE  = 16;
const TK_LOC_COL   = 20;
const TK_TEXT       = 24;
const TK_TEXT_LEN   = 32;
const TK_INT_VAL    = 40;
const TK_BOOL_VAL   = 48;

fn error_category_str(cat: int) -> long {
    switch (cat) {
        case 0: return "lexer";
        case 1: return "parser";
        case 2: return "type";
        case 3: return "sema";
        case 4: return "codegen";
        case 5: return "internal";
        default: return "unknown";
    }
}

fn token_kind_str(kind: int) -> long {
    switch (kind) {
        case 0:  return "INT_LIT";
        case 1:  return "BOOL_LIT";
        case 2:  return "STRING_LIT";
        case 3:  return "FN";
        case 4:  return "RETURN";
        case 5:  return "DEF";
        case 6:  return "IF";
        case 7:  return "ELSE";
        case 8:  return "FOR";
        case 9:  return "WHILE";
        case 10: return "BREAK";
        case 11: return "CONTINUE";
        case 12: return "CONST";
        case 13: return "LET";
        case 14: return "ASM";
        case 15: return "OPP";
        case 16: return "SWITCH";
        case 17: return "CASE";
        case 18: return "DEFAULT";
        case 19: return "TRUE";
        case 20: return "FALSE";
        case 21: return "NULL";
        case 22: return "BIT";
        case 23: return "IMPORT";
        case 24: return "LINK";
        case 25: return "MACRO";
        case 26: return "DEFX";
        case 27: return "STRUCT";
        case 28: return "ENUM";
        case 29: return "MATCH";
        case 30: return "AS";
        case 31: return "IDENT";
        case 32: return "PLUS";
        case 33: return "MINUS";
        case 34: return "STAR";
        case 35: return "SLASH";
        case 36: return "PERCENT";
        case 37: return "ASSIGN";
        case 38: return "EQUAL";
        case 39: return "NOT_EQUAL";
        case 40: return "LESS";
        case 41: return "GREATER";
        case 42: return "LESS_EQ";
        case 43: return "GREATER_EQ";
        case 44: return "AMP";
        case 45: return "PIPE";
        case 46: return "CARET";
        case 47: return "TILDE";
        case 48: return "BANG";
        case 49: return "SHL";
        case 50: return "SHR";
        case 51: return "AND";
        case 52: return "OR";
        case 53: return "PLUS_ASSIGN";
        case 54: return "MINUS_ASSIGN";
        case 55: return "STAR_ASSIGN";
        case 56: return "SLASH_ASSIGN";
        case 57: return "PERCENT_ASSIGN";
        case 58: return "AMP_ASSIGN";
        case 59: return "PIPE_ASSIGN";
        case 60: return "CARET_ASSIGN";
        case 61: return "SHL_ASSIGN";
        case 62: return "SHR_ASSIGN";
        case 63: return "PLUS_PLUS";
        case 64: return "MINUS_MINUS";
        case 65: return "LPAREN";
        case 66: return "RPAREN";
        case 67: return "LBRACE";
        case 68: return "RBRACE";
        case 69: return "LBRACKET";
        case 70: return "RBRACKET";
        case 71: return "SEMICOLON";
        case 72: return "COMMA";
        case 73: return "COLON";
        case 74: return "DOT";
        case 75: return "ARROW";
        case 76: return "ASM_BODY";
        case 77: return "EOF";
        case 78: return "ERROR";
        default: return "UNKNOWN";
    }
}

// token_print(tok: pointer to Token struct)
// Prints: line:col KIND 'text' [value=N]
fn token_print(tok: long) {
    int kind     = mem_read32(tok, TK_KIND);
    int line     = mem_read32(tok, TK_LOC_LINE);
    int col      = mem_read32(tok, TK_LOC_COL);
    long text    = mem_read64(tok, TK_TEXT);
    long text_len = mem_read64(tok, TK_TEXT_LEN);
    long int_val = mem_read64(tok, TK_INT_VAL);
    int bval     = mem_read8(tok, TK_BOOL_VAL);

    long name = token_kind_str(kind);

    // line:col KIND 'text'
    print_int(line);
    print_char(':');
    print_int(col);
    print_char(' ');
    puts(name);
    puts(" '");
    if (text != 0) {
        print_str(text, text_len);
    }
    print_char('\'');

    // value for literals
    if (kind == 0) {
        // TOK_INT_LIT
        puts(" value=");
        print_long(int_val);
    } else {
        if (kind == 1) {
            // TOK_BOOL_LIT
            if (bval != 0) {
                puts(" value=true");
            } else {
                puts(" value=false");
            }
        }
    }
    print_char('\n');
}
