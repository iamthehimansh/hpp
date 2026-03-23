// lexer.c rewritten in H++
// Character-by-character scanner that produces tokens.
// All struct field access via mem_read/write at exact byte offsets.

import std/{sys, mem, str};

def int  bit[32];
def long bit[64];
def byte bit[8];

// ---------------------------------------------------------------------------
//  External functions
// ---------------------------------------------------------------------------
fn arena_alloc(arena: long, size: long) -> long;
fn arena_strndup(arena: long, s: long, len: long) -> long;
fn strtab_intern(tab: long, s: long, len: long) -> long;
fn str_len(s: long) -> long;

// error_report(loc, cat, fmt, ...) — varargs, declare with 6 long params
// loc is a SourceLoc passed by value (filename@rdi, line@rsi=packed?, ...)
// Actually SourceLoc is 16 bytes (ptr+int+int) so passed in rdi,rsi per ABI.
// We pass: loc.filename, loc.line|col packed, cat, fmt, extra1, extra2
// Simplification: error_report takes (SourceLoc loc, int cat, const char *fmt, ...)
// SourceLoc = {filename:ptr, line:int, col:int} = 16 bytes => passed in rdi+rsi
// So ABI: rdi=filename, rsi=line|(col<<32), rdx=cat, rcx=fmt, r8=arg1, r9=arg2
fn error_report(filename: long, line_col: long, cat: int, fmt: long, arg1: long, arg2: long);

// ---------------------------------------------------------------------------
//  Token kind constants (matching token.h enum)
// ---------------------------------------------------------------------------
const TOK_INT_LIT     = 0;
const TOK_BOOL_LIT    = 1;
const TOK_STRING_LIT  = 2;
const TOK_FN          = 3;
const TOK_RETURN      = 4;
const TOK_DEF         = 5;
const TOK_IF          = 6;
const TOK_ELSE        = 7;
const TOK_FOR         = 8;
const TOK_WHILE       = 9;
const TOK_BREAK       = 10;
const TOK_CONTINUE    = 11;
const TOK_CONST       = 12;
const TOK_LET         = 13;
const TOK_ASM         = 14;
const TOK_OPP         = 15;
const TOK_SWITCH      = 16;
const TOK_CASE        = 17;
const TOK_DEFAULT     = 18;
const TOK_TRUE        = 19;
const TOK_FALSE       = 20;
const TOK_NULL        = 21;
const TOK_BIT         = 22;
const TOK_IMPORT      = 23;
const TOK_LINK        = 24;
const TOK_MACRO       = 25;
const TOK_DEFX        = 26;
const TOK_STRUCT      = 27;
const TOK_ENUM        = 28;
const TOK_MATCH       = 29;
const TOK_AS          = 30;
const TOK_IDENT       = 31;
const TOK_PLUS        = 32;
const TOK_MINUS       = 33;
const TOK_STAR        = 34;
const TOK_SLASH       = 35;
const TOK_PERCENT     = 36;
const TOK_ASSIGN      = 37;
const TOK_EQUAL       = 38;
const TOK_NOT_EQUAL   = 39;
const TOK_LESS        = 40;
const TOK_GREATER     = 41;
const TOK_LESS_EQ     = 42;
const TOK_GREATER_EQ  = 43;
const TOK_AMP         = 44;
const TOK_PIPE        = 45;
const TOK_CARET       = 46;
const TOK_TILDE       = 47;
const TOK_BANG        = 48;
const TOK_SHL         = 49;
const TOK_SHR         = 50;
const TOK_AND         = 51;
const TOK_OR          = 52;
const TOK_PLUS_ASSIGN    = 53;
const TOK_MINUS_ASSIGN   = 54;
const TOK_STAR_ASSIGN    = 55;
const TOK_SLASH_ASSIGN   = 56;
const TOK_PERCENT_ASSIGN = 57;
const TOK_AMP_ASSIGN     = 58;
const TOK_PIPE_ASSIGN    = 59;
const TOK_CARET_ASSIGN   = 60;
const TOK_SHL_ASSIGN     = 61;
const TOK_SHR_ASSIGN     = 62;
const TOK_PLUS_PLUS      = 63;
const TOK_MINUS_MINUS    = 64;
const TOK_LPAREN      = 65;
const TOK_RPAREN      = 66;
const TOK_LBRACE      = 67;
const TOK_RBRACE      = 68;
const TOK_LBRACKET    = 69;
const TOK_RBRACKET    = 70;
const TOK_SEMICOLON   = 71;
const TOK_COMMA       = 72;
const TOK_COLON       = 73;
const TOK_DOT         = 74;
const TOK_ARROW       = 75;
const TOK_ASM_BODY    = 76;
const TOK_EOF         = 77;
const TOK_ERROR       = 78;

// Error category
const ERR_LEXER = 0;

// ---------------------------------------------------------------------------
//  Lexer struct layout (136 bytes)
// ---------------------------------------------------------------------------
const LEX_SOURCE     = 0;
const LEX_SOURCE_LEN = 8;
const LEX_FILENAME   = 16;
const LEX_POS        = 24;
const LEX_LINE       = 32;
const LEX_COL        = 36;
const LEX_LINE_START = 40;
const LEX_ARENA      = 48;
const LEX_STRTAB     = 56;
const LEX_HAS_PEEKED = 64;
const LEX_PEEKED_TOK = 72;
const LEX_ASM_STATE  = 128;
const LEX_SIZE       = 136;

// ---------------------------------------------------------------------------
//  Token struct layout (56 bytes)
// ---------------------------------------------------------------------------
const TK_KIND       = 0;
const TK_LOC_FILE   = 8;
const TK_LOC_LINE   = 16;
const TK_LOC_COL    = 20;
const TK_TEXT       = 24;
const TK_TEXT_LEN   = 32;
const TK_INT_VALUE  = 40;
const TK_BOOL_VALUE = 48;
const TK_SIZE       = 56;

// ---------------------------------------------------------------------------
//  Helper: pack line and col into a single long for error_report ABI
// ---------------------------------------------------------------------------
fn pack_line_col(line: int, col: int) -> long {
    long lo = long(line);
    long hi = long(col);
    return lo | (hi << 32);
}

// ---------------------------------------------------------------------------
//  Helper: report_error wrapping error_report with SourceLoc from lexer
// ---------------------------------------------------------------------------
fn lex_error(lex: long, msg: long) {
    long filename = mem_read64(lex, LEX_FILENAME);
    int line = mem_read32(lex, LEX_LINE);
    int col  = mem_read32(lex, LEX_COL);
    long lc = pack_line_col(line, col);
    error_report(filename, lc, ERR_LEXER, msg, 0, 0);
}

fn lex_error_at(filename: long, line: int, col: int, msg: long) {
    long lc = pack_line_col(line, col);
    error_report(filename, lc, ERR_LEXER, msg, 0, 0);
}

// ---------------------------------------------------------------------------
//  Character helpers
// ---------------------------------------------------------------------------
fn is_at_end(lex: long) -> int {
    long pos = mem_read64(lex, LEX_POS);
    long slen = mem_read64(lex, LEX_SOURCE_LEN);
    if (pos >= slen) { return 1; }
    return 0;
}

fn peek_char(lex: long) -> int {
    if (is_at_end(lex) != 0) { return 0; }
    long source = mem_read64(lex, LEX_SOURCE);
    long pos = mem_read64(lex, LEX_POS);
    return mem_read8(source, int(pos));
}

fn peek_char_at(lex: long, offset: long) -> int {
    long pos = mem_read64(lex, LEX_POS);
    long idx = pos + offset;
    long slen = mem_read64(lex, LEX_SOURCE_LEN);
    if (idx >= slen) { return 0; }
    long source = mem_read64(lex, LEX_SOURCE);
    return mem_read8(source, int(idx));
}

fn advance(lex: long) {
    if (is_at_end(lex) != 0) { return; }
    long source = mem_read64(lex, LEX_SOURCE);
    long pos = mem_read64(lex, LEX_POS);
    int ch = mem_read8(source, int(pos));
    if (ch == 10) {
        // newline
        int line = mem_read32(lex, LEX_LINE);
        mem_write32(lex, LEX_LINE, line + 1);
        mem_write32(lex, LEX_COL, 1);
        mem_write64(lex, LEX_POS, pos + 1);
        long new_pos = pos + 1;
        mem_write64(lex, LEX_LINE_START, new_pos);
    } else {
        int col = mem_read32(lex, LEX_COL);
        mem_write32(lex, LEX_COL, col + 1);
        mem_write64(lex, LEX_POS, pos + 1);
    }
}

// ---------------------------------------------------------------------------
//  Character classification helpers
// ---------------------------------------------------------------------------
fn is_digit(c: int) -> int {
    if (c >= 48) { if (c <= 57) { return 1; } }
    return 0;
}

fn is_hex_digit(c: int) -> int {
    if (c >= 48) { if (c <= 57) { return 1; } }
    if (c >= 97) { if (c <= 102) { return 1; } }
    if (c >= 65) { if (c <= 70) { return 1; } }
    return 0;
}

fn is_alpha(c: int) -> int {
    if (c >= 65) { if (c <= 90) { return 1; } }
    if (c >= 97) { if (c <= 122) { return 1; } }
    return 0;
}

fn is_alnum(c: int) -> int {
    if (is_alpha(c) != 0) { return 1; }
    if (is_digit(c) != 0) { return 1; }
    return 0;
}

fn is_ident_char(c: int) -> int {
    if (is_alnum(c) != 0) { return 1; }
    if (c == 95) { return 1; }  // '_'
    return 0;
}

fn is_ident_start(c: int) -> int {
    if (is_alpha(c) != 0) { return 1; }
    if (c == 95) { return 1; }
    return 0;
}

// ---------------------------------------------------------------------------
//  Whitespace / comment skipping
// ---------------------------------------------------------------------------
fn skip_whitespace(lex: long) {
    while (is_at_end(lex) == 0) {
        int c = peek_char(lex);
        if (c == 32) { advance(lex); }       // space
        else { if (c == 9) { advance(lex); }  // tab
        else { if (c == 10) { advance(lex); } // newline
        else { if (c == 13) { advance(lex); } // carriage return
        else { return; } } } }
    }
}

fn skip_line_comment(lex: long) {
    while (is_at_end(lex) == 0) {
        int c = peek_char(lex);
        if (c == 10) { return; }
        advance(lex);
    }
}

fn skip_block_comment(lex: long) {
    long filename = mem_read64(lex, LEX_FILENAME);
    int start_line = mem_read32(lex, LEX_LINE);
    int start_col  = mem_read32(lex, LEX_COL);
    while (is_at_end(lex) == 0) {
        int c = peek_char(lex);
        if (c == 42) {
            // '*'
            int c2 = peek_char_at(lex, 1);
            if (c2 == 47) {
                // '/'
                advance(lex);
                advance(lex);
                return;
            }
        }
        advance(lex);
    }
    lex_error_at(filename, start_line, start_col, "unterminated block comment");
}

// ---------------------------------------------------------------------------
//  Write a zero-initialized token to a pointer
// ---------------------------------------------------------------------------
fn token_clear(tok: long) {
    mem_zero(tok, TK_SIZE);
}

// ---------------------------------------------------------------------------
//  Write location from current lexer state into a token
// ---------------------------------------------------------------------------
fn token_set_loc(tok: long, lex: long) {
    long filename = mem_read64(lex, LEX_FILENAME);
    int line = mem_read32(lex, LEX_LINE);
    int col  = mem_read32(lex, LEX_COL);
    mem_write64(tok, TK_LOC_FILE, filename);
    mem_write32(tok, TK_LOC_LINE, line);
    mem_write32(tok, TK_LOC_COL, col);
}

// ---------------------------------------------------------------------------
//  Keyword matching: compare len bytes at src against a null-terminated kw
// ---------------------------------------------------------------------------
fn kw_match(src: long, len: long, kw: long) -> int {
    long kw_len = str_len(kw);
    if (kw_len != len) { return 0; }
    int cmp = mem_cmp(kw, src, len);
    if (cmp == 0) { return 1; }
    return 0;
}

// Try all keywords, return the TokenKind or -1 if not a keyword
fn lookup_keyword(src: long, len: long) -> int {
    if (kw_match(src, len, "fn") != 0)       { return TOK_FN; }
    if (kw_match(src, len, "return") != 0)   { return TOK_RETURN; }
    if (kw_match(src, len, "def") != 0)      { return TOK_DEF; }
    if (kw_match(src, len, "if") != 0)       { return TOK_IF; }
    if (kw_match(src, len, "else") != 0)     { return TOK_ELSE; }
    if (kw_match(src, len, "for") != 0)      { return TOK_FOR; }
    if (kw_match(src, len, "while") != 0)    { return TOK_WHILE; }
    if (kw_match(src, len, "break") != 0)    { return TOK_BREAK; }
    if (kw_match(src, len, "continue") != 0) { return TOK_CONTINUE; }
    if (kw_match(src, len, "const") != 0)    { return TOK_CONST; }
    if (kw_match(src, len, "let") != 0)      { return TOK_LET; }
    if (kw_match(src, len, "asm") != 0)      { return TOK_ASM; }
    if (kw_match(src, len, "opp") != 0)      { return TOK_OPP; }
    if (kw_match(src, len, "true") != 0)     { return TOK_TRUE; }
    if (kw_match(src, len, "false") != 0)    { return TOK_FALSE; }
    if (kw_match(src, len, "null") != 0)     { return TOK_NULL; }
    if (kw_match(src, len, "bit") != 0)      { return TOK_BIT; }
    if (kw_match(src, len, "import") != 0)   { return TOK_IMPORT; }
    if (kw_match(src, len, "link") != 0)     { return TOK_LINK; }
    if (kw_match(src, len, "macro") != 0)    { return TOK_MACRO; }
    if (kw_match(src, len, "defx") != 0)     { return TOK_DEFX; }
    if (kw_match(src, len, "switch") != 0)   { return TOK_SWITCH; }
    if (kw_match(src, len, "case") != 0)     { return TOK_CASE; }
    if (kw_match(src, len, "default") != 0)  { return TOK_DEFAULT; }
    if (kw_match(src, len, "struct") != 0)   { return TOK_STRUCT; }
    if (kw_match(src, len, "enum") != 0)     { return TOK_ENUM; }
    if (kw_match(src, len, "match") != 0)    { return TOK_MATCH; }
    if (kw_match(src, len, "as") != 0)       { return TOK_AS; }
    return 999;  // sentinel: not a keyword
}

// ---------------------------------------------------------------------------
//  lex_number: decimal, hex, binary, octal
// ---------------------------------------------------------------------------
fn lex_number(tok: long, lex: long) {
    token_clear(tok);
    mem_write32(tok, TK_KIND, TOK_INT_LIT);
    token_set_loc(tok, lex);

    long source = mem_read64(lex, LEX_SOURCE);
    long start_pos = mem_read64(lex, LEX_POS);
    long start = source + start_pos;
    long value = 0;

    int c0 = peek_char(lex);
    int c1 = peek_char_at(lex, 1);

    if (c0 == 48) {
        // '0' prefix
        if (c1 == 120 || c1 == 88) {
            // 0x / 0X — hex
            advance(lex);
            advance(lex);
            int hc = peek_char(lex);
            if (is_hex_digit(hc) == 0) {
                lex_error(lex, "expected hex digit after '0x'");
                mem_write32(tok, TK_KIND, TOK_ERROR);
            }
            while (is_hex_digit(peek_char(lex)) != 0) {
                int d = peek_char(lex);
                long digit = 0;
                if (d >= 48) { if (d <= 57) { digit = long(d - 48); } }
                if (d >= 97) { if (d <= 102) { digit = long(d - 97 + 10); } }
                if (d >= 65) { if (d <= 70) { digit = long(d - 65 + 10); } }
                value = value * 16 + digit;
                advance(lex);
            }
        } else {
            if (c1 == 98 || c1 == 66) {
                // 0b / 0B — binary
                advance(lex);
                advance(lex);
                int bc = peek_char(lex);
                if (bc != 48) {
                    if (bc != 49) {
                        lex_error(lex, "expected binary digit after '0b'");
                        mem_write32(tok, TK_KIND, TOK_ERROR);
                    }
                }
                while (peek_char(lex) == 48 || peek_char(lex) == 49) {
                    long bd = long(peek_char(lex) - 48);
                    value = value * 2 + bd;
                    advance(lex);
                }
                if (is_digit(peek_char(lex)) != 0) {
                    lex_error(lex, "invalid digit in binary literal");
                    mem_write32(tok, TK_KIND, TOK_ERROR);
                    while (is_digit(peek_char(lex)) != 0) { advance(lex); }
                }
            } else {
                if (c1 == 111 || c1 == 79) {
                    // 0o / 0O — octal
                    advance(lex);
                    advance(lex);
                    int oc = peek_char(lex);
                    if (oc < 48 || oc > 55) {
                        lex_error(lex, "expected octal digit after '0o'");
                        mem_write32(tok, TK_KIND, TOK_ERROR);
                    }
                    while (peek_char(lex) >= 48) {
                        if (peek_char(lex) > 55) { break; }
                        long od = long(peek_char(lex) - 48);
                        value = value * 8 + od;
                        advance(lex);
                    }
                    if (is_digit(peek_char(lex)) != 0) {
                        lex_error(lex, "invalid digit in octal literal");
                        mem_write32(tok, TK_KIND, TOK_ERROR);
                        while (is_digit(peek_char(lex)) != 0) { advance(lex); }
                    }
                } else {
                    // Decimal starting with 0
                    while (is_digit(peek_char(lex)) != 0) {
                        long dd = long(peek_char(lex) - 48);
                        value = value * 10 + dd;
                        advance(lex);
                    }
                }
            }
        }
    } else {
        // Decimal
        while (is_digit(peek_char(lex)) != 0) {
            long dd = long(peek_char(lex) - 48);
            value = value * 10 + dd;
            advance(lex);
        }
    }

    long end_pos = mem_read64(lex, LEX_POS);
    long len = end_pos - start_pos;
    long strtab = mem_read64(lex, LEX_STRTAB);
    long text = strtab_intern(strtab, start, len);
    mem_write64(tok, TK_TEXT, text);
    mem_write64(tok, TK_TEXT_LEN, len);
    mem_write64(tok, TK_INT_VALUE, value);
}

// ---------------------------------------------------------------------------
//  lex_identifier_or_keyword
// ---------------------------------------------------------------------------
fn lex_identifier_or_keyword(tok: long, lex: long) {
    token_clear(tok);
    token_set_loc(tok, lex);

    long source = mem_read64(lex, LEX_SOURCE);
    long start_pos = mem_read64(lex, LEX_POS);
    long start = source + start_pos;

    while (is_ident_char(peek_char(lex)) != 0) {
        advance(lex);
    }

    long end_pos = mem_read64(lex, LEX_POS);
    long len = end_pos - start_pos;

    int kw = lookup_keyword(start, len);
    if (kw != 999) {
        // It's a keyword
        long strtab = mem_read64(lex, LEX_STRTAB);
        long text = strtab_intern(strtab, start, len);
        mem_write32(tok, TK_KIND, kw);
        mem_write64(tok, TK_TEXT, text);
        mem_write64(tok, TK_TEXT_LEN, len);

        if (kw == TOK_TRUE) {
            mem_write32(tok, TK_KIND, TOK_BOOL_LIT);
            mem_write8(tok, TK_BOOL_VALUE, 1);
        } else {
            if (kw == TOK_FALSE) {
                mem_write32(tok, TK_KIND, TOK_BOOL_LIT);
                mem_write8(tok, TK_BOOL_VALUE, 0);
            }
        }
    } else {
        // Identifier
        long strtab = mem_read64(lex, LEX_STRTAB);
        long text = strtab_intern(strtab, start, len);
        mem_write32(tok, TK_KIND, TOK_IDENT);
        mem_write64(tok, TK_TEXT, text);
        mem_write64(tok, TK_TEXT_LEN, len);
    }
}

// ---------------------------------------------------------------------------
//  make_token: simple 1/2/3 character token
// ---------------------------------------------------------------------------
fn make_token(tok: long, lex: long, kind: int, char_count: long) {
    token_clear(tok);
    token_set_loc(tok, lex);
    mem_write32(tok, TK_KIND, kind);

    long source = mem_read64(lex, LEX_SOURCE);
    long pos = mem_read64(lex, LEX_POS);
    long start = source + pos;

    long i = 0;
    while (i < char_count) {
        advance(lex);
        i = i + 1;
    }

    long strtab = mem_read64(lex, LEX_STRTAB);
    long text = strtab_intern(strtab, start, char_count);
    mem_write64(tok, TK_TEXT, text);
    mem_write64(tok, TK_TEXT_LEN, char_count);
}

// ---------------------------------------------------------------------------
//  lex_char_literal: 'a' -> int value
// ---------------------------------------------------------------------------
fn lex_char_literal(tok: long, lex: long) {
    token_clear(tok);
    token_set_loc(tok, lex);
    advance(lex); // skip opening quote

    int ch = 0;
    int pc = peek_char(lex);
    if (pc == 92) {
        // backslash escape
        advance(lex);
        int esc = peek_char(lex);
        advance(lex);
        if (esc == 110) { ch = 10; }       // \n
        else { if (esc == 116) { ch = 9; } // \t
        else { if (esc == 114) { ch = 13; } // \r
        else { if (esc == 48) { ch = 0; }   // \0
        else { if (esc == 92) { ch = 92; }  // \\
        else { if (esc == 39) { ch = 39; }  // \'
        else { ch = esc; } } } } } }
    } else {
        ch = pc;
        advance(lex);
    }

    int closing = peek_char(lex);
    if (closing != 39) {
        // not a closing quote
        lex_error(lex, "unterminated char literal");
        mem_write32(tok, TK_KIND, TOK_ERROR);
        mem_write64(tok, TK_TEXT, "");
        mem_write64(tok, TK_TEXT_LEN, 0);
        return;
    }
    advance(lex); // skip closing quote

    mem_write32(tok, TK_KIND, TOK_INT_LIT);

    // Intern the single char: write ch to a temp buffer on stack
    // We need a 1-byte string. Use strtab_intern with the source pointer.
    // Actually, allocate a small buffer from arena for the text.
    long arena = mem_read64(lex, LEX_ARENA);
    long buf = arena_alloc(arena, 2);
    mem_write8(buf, 0, ch);
    mem_write8(buf, 1, 0);
    long strtab = mem_read64(lex, LEX_STRTAB);
    long text = strtab_intern(strtab, buf, 1);
    mem_write64(tok, TK_TEXT, text);
    mem_write64(tok, TK_TEXT_LEN, 1);
    mem_write64(tok, TK_INT_VALUE, long(ch));
}

// ---------------------------------------------------------------------------
//  lex_string_literal: "..." with escape sequences
// ---------------------------------------------------------------------------
fn lex_string_literal(tok: long, lex: long) {
    token_clear(tok);
    token_set_loc(tok, lex);
    advance(lex); // skip opening quote

    // First pass: compute raw_len and out_len
    long raw_start_pos = mem_read64(lex, LEX_POS);
    long raw_len = 0;
    long out_len = 0;

    while (is_at_end(lex) == 0) {
        int c = peek_char(lex);
        if (c == 34) { break; }   // closing "
        if (c == 10) { break; }   // newline => unterminated
        if (c == 92) {
            // backslash
            advance(lex);
            raw_len = raw_len + 1;
        }
        advance(lex);
        raw_len = raw_len + 1;
        out_len = out_len + 1;
    }

    int end_c = peek_char(lex);
    if (is_at_end(lex) != 0 || end_c != 34) {
        lex_error(lex, "unterminated string literal");
        mem_write32(tok, TK_KIND, TOK_ERROR);
        mem_write64(tok, TK_TEXT, "");
        mem_write64(tok, TK_TEXT_LEN, 0);
        return;
    }
    advance(lex); // skip closing quote

    // Second pass: build output buffer with escapes processed
    long arena = mem_read64(lex, LEX_ARENA);
    long buf = arena_alloc(arena, out_len + 1);
    long source = mem_read64(lex, LEX_SOURCE);
    long src = source + raw_start_pos;

    long i = 0;
    long j = 0;
    while (i < raw_len) {
        if (j >= out_len) { break; }
        int sc = mem_read8(src, int(i));
        if (sc == 92) {
            // backslash
            i = i + 1;
            if (i < raw_len) {
                int ec = mem_read8(src, int(i));
                if (ec == 110) { mem_write8(buf, int(j), 10); }       // \n
                else { if (ec == 116) { mem_write8(buf, int(j), 9); } // \t
                else { if (ec == 114) { mem_write8(buf, int(j), 13); } // \r
                else { if (ec == 48) { mem_write8(buf, int(j), 0); }   // \0
                else { if (ec == 92) { mem_write8(buf, int(j), 92); }  // \\
                else { if (ec == 34) { mem_write8(buf, int(j), 34); }  // \"
                else { mem_write8(buf, int(j), ec); } } } } } }
                i = i + 1;
                j = j + 1;
            }
        } else {
            mem_write8(buf, int(j), sc);
            i = i + 1;
            j = j + 1;
        }
    }
    mem_write8(buf, int(j), 0); // null terminator

    mem_write32(tok, TK_KIND, TOK_STRING_LIT);
    mem_write64(tok, TK_TEXT, buf);
    mem_write64(tok, TK_TEXT_LEN, j);
}

// ---------------------------------------------------------------------------
//  lex_asm_body: collect raw text until matching '}'
// ---------------------------------------------------------------------------
fn lex_asm_body(tok: long, lex: long) {
    skip_whitespace(lex);
    token_clear(tok);
    token_set_loc(tok, lex);

    long start_pos = mem_read64(lex, LEX_POS);
    int depth = 1;
    while (is_at_end(lex) == 0) {
        if (depth <= 0) { break; }
        int c = peek_char(lex);
        if (c == 123) {
            // '{'
            depth = depth + 1;
        } else {
            if (c == 125) {
                // '}'
                depth = depth - 1;
                if (depth == 0) { break; }
            }
        }
        advance(lex);
    }

    long end_pos = mem_read64(lex, LEX_POS);
    long source = mem_read64(lex, LEX_SOURCE);

    // Trim trailing whitespace
    while (end_pos > start_pos) {
        int tc = mem_read8(source, int(end_pos - 1));
        if (tc == 32 || tc == 9 || tc == 10 || tc == 13) {
            end_pos = end_pos - 1;
        } else {
            break;
        }
    }

    long len = end_pos - start_pos;
    long arena = mem_read64(lex, LEX_ARENA);
    long body = arena_strndup(arena, source + start_pos, len);

    mem_write32(tok, TK_KIND, TOK_ASM_BODY);
    mem_write64(tok, TK_TEXT, body);
    mem_write64(tok, TK_TEXT_LEN, len);
    mem_write32(lex, LEX_ASM_STATE, 0);
}

// ---------------------------------------------------------------------------
//  update_asm_state: track asm <target> { state machine
// ---------------------------------------------------------------------------
fn update_asm_state(lex: long, tok: long) {
    int kind = mem_read32(tok, TK_KIND);
    int state = mem_read32(lex, LEX_ASM_STATE);

    if (kind == TOK_ASM) {
        mem_write32(lex, LEX_ASM_STATE, 1);
    } else {
        if (state == 1) {
            if (kind == TOK_IDENT) {
                mem_write32(lex, LEX_ASM_STATE, 2);
            } else {
                mem_write32(lex, LEX_ASM_STATE, 0);
            }
        } else {
            if (state == 2) {
                if (kind == TOK_LBRACE) {
                    mem_write32(lex, LEX_ASM_STATE, 3);
                } else {
                    mem_write32(lex, LEX_ASM_STATE, 0);
                }
            } else {
                mem_write32(lex, LEX_ASM_STATE, 0);
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  copy_token: copy TK_SIZE bytes from src to dst
// ---------------------------------------------------------------------------
fn copy_token(dst: long, src: long) {
    mem_copy(dst, src, TK_SIZE);
}

// ---------------------------------------------------------------------------
//  lexer_next_raw: the main scanner loop
// ---------------------------------------------------------------------------
fn lexer_next_raw(tok: long, lex: long) {
    for (;;) {
        skip_whitespace(lex);

        if (is_at_end(lex) != 0) {
            token_clear(tok);
            mem_write32(tok, TK_KIND, TOK_EOF);
            token_set_loc(tok, lex);
            mem_write64(tok, TK_TEXT, "");
            mem_write64(tok, TK_TEXT_LEN, 0);
            return;
        }

        int c = peek_char(lex);

        // Line comment //
        if (c == 47) {
            int c2 = peek_char_at(lex, 1);
            if (c2 == 47) {
                advance(lex);
                advance(lex);
                skip_line_comment(lex);
                continue;
            }
            if (c2 == 42) {
                // Block comment /*
                advance(lex);
                advance(lex);
                skip_block_comment(lex);
                continue;
            }
        }

        // Numbers
        if (is_digit(c) != 0) {
            lex_number(tok, lex);
            return;
        }

        // Identifiers / keywords
        if (is_ident_start(c) != 0) {
            lex_identifier_or_keyword(tok, lex);
            return;
        }

        // Operators and punctuation
        if (c == 61) {
            // '='
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_EQUAL, 2); return; }
            make_token(tok, lex, TOK_ASSIGN, 1); return;
        }
        if (c == 33) {
            // '!'
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_NOT_EQUAL, 2); return; }
            make_token(tok, lex, TOK_BANG, 1); return;
        }
        if (c == 60) {
            // '<'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 60) {
                int n2 = peek_char_at(lex, 2);
                if (n2 == 61) { make_token(tok, lex, TOK_SHL_ASSIGN, 3); return; }
                make_token(tok, lex, TOK_SHL, 2); return;
            }
            if (n1 == 61) { make_token(tok, lex, TOK_LESS_EQ, 2); return; }
            make_token(tok, lex, TOK_LESS, 1); return;
        }
        if (c == 62) {
            // '>'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 62) {
                int n2 = peek_char_at(lex, 2);
                if (n2 == 61) { make_token(tok, lex, TOK_SHR_ASSIGN, 3); return; }
                make_token(tok, lex, TOK_SHR, 2); return;
            }
            if (n1 == 61) { make_token(tok, lex, TOK_GREATER_EQ, 2); return; }
            make_token(tok, lex, TOK_GREATER, 1); return;
        }
        if (c == 38) {
            // '&'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 38) { make_token(tok, lex, TOK_AND, 2); return; }
            if (n1 == 61) { make_token(tok, lex, TOK_AMP_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_AMP, 1); return;
        }
        if (c == 124) {
            // '|'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 124) { make_token(tok, lex, TOK_OR, 2); return; }
            if (n1 == 61) { make_token(tok, lex, TOK_PIPE_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_PIPE, 1); return;
        }
        if (c == 45) {
            // '-'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 62) { make_token(tok, lex, TOK_ARROW, 2); return; }
            if (n1 == 45) { make_token(tok, lex, TOK_MINUS_MINUS, 2); return; }
            if (n1 == 61) { make_token(tok, lex, TOK_MINUS_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_MINUS, 1); return;
        }
        if (c == 43) {
            // '+'
            int n1 = peek_char_at(lex, 1);
            if (n1 == 43) { make_token(tok, lex, TOK_PLUS_PLUS, 2); return; }
            if (n1 == 61) { make_token(tok, lex, TOK_PLUS_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_PLUS, 1); return;
        }
        if (c == 42) {
            // '*'
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_STAR_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_STAR, 1); return;
        }
        if (c == 47) {
            // '/' (already checked // and /* above)
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_SLASH_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_SLASH, 1); return;
        }
        if (c == 37) {
            // '%'
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_PERCENT_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_PERCENT, 1); return;
        }
        if (c == 94) {
            // '^'
            if (peek_char_at(lex, 1) == 61) { make_token(tok, lex, TOK_CARET_ASSIGN, 2); return; }
            make_token(tok, lex, TOK_CARET, 1); return;
        }
        if (c == 126) { make_token(tok, lex, TOK_TILDE, 1); return; }

        // Punctuation
        if (c == 40) { make_token(tok, lex, TOK_LPAREN, 1); return; }
        if (c == 41) { make_token(tok, lex, TOK_RPAREN, 1); return; }
        if (c == 123) { make_token(tok, lex, TOK_LBRACE, 1); return; }
        if (c == 125) { make_token(tok, lex, TOK_RBRACE, 1); return; }
        if (c == 91) { make_token(tok, lex, TOK_LBRACKET, 1); return; }
        if (c == 93) { make_token(tok, lex, TOK_RBRACKET, 1); return; }
        if (c == 59) { make_token(tok, lex, TOK_SEMICOLON, 1); return; }
        if (c == 44) { make_token(tok, lex, TOK_COMMA, 1); return; }
        if (c == 58) { make_token(tok, lex, TOK_COLON, 1); return; }
        if (c == 46) { make_token(tok, lex, TOK_DOT, 1); return; }

        // Char literal
        if (c == 39) {
            lex_char_literal(tok, lex);
            return;
        }

        // String literal
        if (c == 34) {
            lex_string_literal(tok, lex);
            return;
        }

        // Unknown character
        token_clear(tok);
        mem_write32(tok, TK_KIND, TOK_ERROR);
        token_set_loc(tok, lex);
        long source = mem_read64(lex, LEX_SOURCE);
        long pos = mem_read64(lex, LEX_POS);
        long strtab = mem_read64(lex, LEX_STRTAB);
        long text = strtab_intern(strtab, source + pos, 1);
        mem_write64(tok, TK_TEXT, text);
        mem_write64(tok, TK_TEXT_LEN, 1);
        lex_error(lex, "unexpected character");
        advance(lex);
        return;
    }
}

// ---------------------------------------------------------------------------
//  Public API: lexer_init
// ---------------------------------------------------------------------------
fn lexer_init(lex: long, source: long, source_len: long,
              filename: long, arena: long, strtab: long) {
    mem_write64(lex, LEX_SOURCE, source);
    mem_write64(lex, LEX_SOURCE_LEN, source_len);
    mem_write64(lex, LEX_FILENAME, filename);
    mem_write64(lex, LEX_POS, 0);
    mem_write32(lex, LEX_LINE, 1);
    mem_write32(lex, LEX_COL, 1);
    mem_write64(lex, LEX_LINE_START, 0);
    mem_write64(lex, LEX_ARENA, arena);
    mem_write64(lex, LEX_STRTAB, strtab);
    mem_write8(lex, LEX_HAS_PEEKED, 0);
    mem_zero(lex + LEX_PEEKED_TOK, TK_SIZE);
    mem_write32(lex, LEX_ASM_STATE, 0);
}

// ---------------------------------------------------------------------------
//  Public API: lexer_next (hidden return pointer ABI)
//  C ABI: Token lexer_next(Lexer *lex)
//  Since Token is 56 bytes (>16), the caller passes a hidden pointer in rdi.
//  H++ signature: lexer_next(ret: long, lex: long) -> long
//  We write the token to [ret] and return ret.
// ---------------------------------------------------------------------------
fn lexer_next(ret: long, lex: long) -> long {
    // Check if we have a peeked token
    int has_peeked = mem_read8(lex, LEX_HAS_PEEKED);
    if (has_peeked != 0) {
        mem_write8(lex, LEX_HAS_PEEKED, 0);
        long peeked = lex + LEX_PEEKED_TOK;
        copy_token(ret, peeked);
        update_asm_state(lex, ret);
        return ret;
    }

    // If asm_state == 3, collect raw body
    int asm_state = mem_read32(lex, LEX_ASM_STATE);
    if (asm_state == 3) {
        lex_asm_body(ret, lex);
        return ret;
    }

    // Normal scan
    lexer_next_raw(ret, lex);
    update_asm_state(lex, ret);
    return ret;
}

// ---------------------------------------------------------------------------
//  Public API: lexer_peek (hidden return pointer ABI)
//  Same ABI as lexer_next.
// ---------------------------------------------------------------------------
fn lexer_peek(ret: long, lex: long) -> long {
    int has_peeked = mem_read8(lex, LEX_HAS_PEEKED);
    if (has_peeked == 0) {
        // Call lexer_next to fill peeked_token
        long peeked = lex + LEX_PEEKED_TOK;
        lexer_next(peeked, lex);
        mem_write8(lex, LEX_HAS_PEEKED, 1);
    }
    // Copy peeked token to return pointer
    long peeked = lex + LEX_PEEKED_TOK;
    copy_token(ret, peeked);
    return ret;
}

// ---------------------------------------------------------------------------
//  Public API: lexer_lex_all(lex, out_tokens_ptr) -> int
//  Allocates a Token array from the arena, returns count.
// ---------------------------------------------------------------------------
fn lexer_lex_all(lex: long, out: long) -> int {
    int capacity = 256;
    int count = 0;
    long arena = mem_read64(lex, LEX_ARENA);
    long tokens = arena_alloc(arena, long(capacity) * TK_SIZE);

    for (;;) {
        if (count >= capacity) {
            int new_cap = capacity * 2;
            long bigger = arena_alloc(arena, long(new_cap) * TK_SIZE);
            mem_copy(bigger, tokens, long(count) * TK_SIZE);
            tokens = bigger;
            capacity = new_cap;
        }

        long tok_ptr = tokens + long(count) * TK_SIZE;
        lexer_next(tok_ptr, lex);
        int kind = mem_read32(tok_ptr, TK_KIND);
        if (kind == TOK_EOF) {
            count = count + 1;
            break;
        }
        count = count + 1;
    }

    mem_write64(out, 0, tokens);
    return count;
}
