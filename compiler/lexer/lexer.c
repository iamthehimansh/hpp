#include "lexer/lexer.h"
#include "common/error.h"
#include "common/arena.h"
#include "common/strtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Keyword table                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *text;
    TokenKind   kind;
} Keyword;

static const Keyword keywords[] = {
    { "fn",       TOK_FN       },
    { "return",   TOK_RETURN   },
    { "def",      TOK_DEF      },
    { "if",       TOK_IF       },
    { "else",     TOK_ELSE     },
    { "for",      TOK_FOR      },
    { "while",    TOK_WHILE    },
    { "break",    TOK_BREAK    },
    { "continue", TOK_CONTINUE },
    { "const",    TOK_CONST    },
    { "let",      TOK_LET      },
    { "asm",      TOK_ASM      },
    { "opp",      TOK_OPP      },
    { "true",     TOK_TRUE     },
    { "false",    TOK_FALSE    },
    { "null",     TOK_NULL     },
    { "bit",      TOK_BIT      },
    { "import",   TOK_IMPORT   },
    { "link",     TOK_LINK     },
    { "macro",    TOK_MACRO    },
    { "defx",     TOK_DEFX     },
    { "switch",   TOK_SWITCH   },
    { "case",     TOK_CASE     },
    { "default",  TOK_DEFAULT  },
    { "struct",   TOK_STRUCT   },
    { "enum",     TOK_ENUM     },
    { "match",    TOK_MATCH    },
    { "as",       TOK_AS       },
};

#define KEYWORD_COUNT ((int)(sizeof(keywords) / sizeof(keywords[0])))

/* ------------------------------------------------------------------ */
/*  Character helpers                                                  */
/* ------------------------------------------------------------------ */

static inline bool is_at_end(const Lexer *lex)
{
    return lex->pos >= lex->source_len;
}

static inline char peek_char(const Lexer *lex)
{
    if (is_at_end(lex)) return '\0';
    return lex->source[lex->pos];
}

static inline char peek_char_at(const Lexer *lex, size_t offset)
{
    size_t idx = lex->pos + offset;
    if (idx >= lex->source_len) return '\0';
    return lex->source[idx];
}

static inline void advance(Lexer *lex)
{
    if (is_at_end(lex)) return;
    if (lex->source[lex->pos] == '\n') {
        lex->line++;
        lex->col = 1;
        lex->pos++;
        lex->line_start = lex->pos;
    } else {
        lex->col++;
        lex->pos++;
    }
}

static inline SourceLoc make_loc(const Lexer *lex)
{
    SourceLoc loc;
    loc.filename = lex->filename;
    loc.line     = lex->line;
    loc.col      = lex->col;
    return loc;
}

/* ------------------------------------------------------------------ */
/*  Whitespace and comment skipping                                    */
/* ------------------------------------------------------------------ */

static void skip_whitespace(Lexer *lex)
{
    while (!is_at_end(lex)) {
        char c = peek_char(lex);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(lex);
        } else {
            break;
        }
    }
}

static void skip_line_comment(Lexer *lex)
{
    /* Already past the '//' */
    while (!is_at_end(lex) && peek_char(lex) != '\n') {
        advance(lex);
    }
}

static void skip_block_comment(Lexer *lex)
{
    /* Already past the opening delimiter */
    SourceLoc start_loc = make_loc(lex);
    while (!is_at_end(lex)) {
        if (peek_char(lex) == '*' && peek_char_at(lex, 1) == '/') {
            advance(lex); /* skip '*' */
            advance(lex); /* skip '/' */
            return;
        }
        advance(lex);
    }
    error_report(start_loc, ERR_LEXER, "unterminated block comment");
}

/* ------------------------------------------------------------------ */
/*  Number lexing                                                      */
/* ------------------------------------------------------------------ */

static Token lex_number(Lexer *lex)
{
    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.kind = TOK_INT_LIT;
    tok.loc  = make_loc(lex);

    const char *start = lex->source + lex->pos;
    uint64_t value = 0;

    if (peek_char(lex) == '0' && (peek_char_at(lex, 1) == 'x' || peek_char_at(lex, 1) == 'X')) {
        /* Hexadecimal */
        advance(lex); /* '0' */
        advance(lex); /* 'x' */
        if (!isxdigit((unsigned char)peek_char(lex))) {
            error_report(tok.loc, ERR_LEXER, "expected hex digit after '0x'");
            tok.kind = TOK_ERROR;
        }
        while (isxdigit((unsigned char)peek_char(lex))) {
            char c = peek_char(lex);
            uint64_t digit;
            if (c >= '0' && c <= '9')      digit = (uint64_t)(c - '0');
            else if (c >= 'a' && c <= 'f') digit = (uint64_t)(c - 'a' + 10);
            else                           digit = (uint64_t)(c - 'A' + 10);
            value = value * 16 + digit;
            advance(lex);
        }
    } else if (peek_char(lex) == '0' && (peek_char_at(lex, 1) == 'b' || peek_char_at(lex, 1) == 'B')) {
        /* Binary */
        advance(lex); /* '0' */
        advance(lex); /* 'b' */
        if (peek_char(lex) != '0' && peek_char(lex) != '1') {
            error_report(tok.loc, ERR_LEXER, "expected binary digit after '0b'");
            tok.kind = TOK_ERROR;
        }
        while (peek_char(lex) == '0' || peek_char(lex) == '1') {
            value = value * 2 + (uint64_t)(peek_char(lex) - '0');
            advance(lex);
        }
        if (isdigit((unsigned char)peek_char(lex))) {
            error_report(tok.loc, ERR_LEXER, "invalid digit in binary literal");
            tok.kind = TOK_ERROR;
            while (isdigit((unsigned char)peek_char(lex))) advance(lex);
        }
    } else if (peek_char(lex) == '0' && (peek_char_at(lex, 1) == 'o' || peek_char_at(lex, 1) == 'O')) {
        /* Octal */
        advance(lex); /* '0' */
        advance(lex); /* 'o' */
        if (peek_char(lex) < '0' || peek_char(lex) > '7') {
            error_report(tok.loc, ERR_LEXER, "expected octal digit after '0o'");
            tok.kind = TOK_ERROR;
        }
        while (peek_char(lex) >= '0' && peek_char(lex) <= '7') {
            value = value * 8 + (uint64_t)(peek_char(lex) - '0');
            advance(lex);
        }
        if (isdigit((unsigned char)peek_char(lex))) {
            error_report(tok.loc, ERR_LEXER, "invalid digit in octal literal");
            tok.kind = TOK_ERROR;
            while (isdigit((unsigned char)peek_char(lex))) advance(lex);
        }
    } else {
        /* Decimal */
        while (isdigit((unsigned char)peek_char(lex))) {
            value = value * 10 + (uint64_t)(peek_char(lex) - '0');
            advance(lex);
        }
    }

    size_t len = (size_t)(lex->source + lex->pos - start);
    tok.text      = strtab_intern(lex->strtab, start, len);
    tok.text_len  = len;
    tok.int_value = value;
    return tok;
}

/* ------------------------------------------------------------------ */
/*  Identifier / keyword lexing                                        */
/* ------------------------------------------------------------------ */

static Token lex_identifier_or_keyword(Lexer *lex)
{
    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.loc = make_loc(lex);

    const char *start = lex->source + lex->pos;

    while (isalnum((unsigned char)peek_char(lex)) || peek_char(lex) == '_') {
        advance(lex);
    }

    size_t len = (size_t)(lex->source + lex->pos - start);

    /* Check keywords */
    for (int i = 0; i < KEYWORD_COUNT; i++) {
        if (strlen(keywords[i].text) == len &&
            memcmp(keywords[i].text, start, len) == 0) {
            tok.kind     = keywords[i].kind;
            tok.text     = strtab_intern(lex->strtab, start, len);
            tok.text_len = len;

            if (tok.kind == TOK_TRUE) {
                tok.kind       = TOK_BOOL_LIT;
                tok.bool_value = true;
            } else if (tok.kind == TOK_FALSE) {
                tok.kind       = TOK_BOOL_LIT;
                tok.bool_value = false;
            }
            return tok;
        }
    }

    tok.kind     = TOK_IDENT;
    tok.text     = strtab_intern(lex->strtab, start, len);
    tok.text_len = len;
    return tok;
}

/* ------------------------------------------------------------------ */
/*  Make a simple token (1 or 2 chars)                                 */
/* ------------------------------------------------------------------ */

static Token make_token(Lexer *lex, TokenKind kind, size_t char_count)
{
    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.loc  = make_loc(lex);
    tok.kind = kind;

    const char *start = lex->source + lex->pos;
    for (size_t i = 0; i < char_count; i++) {
        advance(lex);
    }
    tok.text     = strtab_intern(lex->strtab, start, char_count);
    tok.text_len = char_count;
    return tok;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void lexer_init(Lexer *lex, const char *source, size_t len,
                const char *filename, Arena *arena, StrTab *strtab)
{
    lex->source      = source;
    lex->source_len  = len;
    lex->filename    = filename;
    lex->pos         = 0;
    lex->line        = 1;
    lex->col         = 1;
    lex->line_start  = 0;
    lex->arena       = arena;
    lex->strtab      = strtab;
    lex->has_peeked  = false;
    memset(&lex->peeked_token, 0, sizeof(lex->peeked_token));
}

static Token lex_asm_body(Lexer *lex)
{
    /* Collect raw text until matching '}', tracking brace depth */
    skip_whitespace(lex);
    SourceLoc loc = make_loc(lex);
    size_t start = lex->pos;
    int depth = 1;
    while (!is_at_end(lex) && depth > 0) {
        char c = peek_char(lex);
        if (c == '{') depth++;
        else if (c == '}') {
            depth--;
            if (depth == 0) break;
        }
        advance(lex);
    }
    size_t end = lex->pos;
    /* Trim trailing whitespace */
    while (end > start && (lex->source[end-1] == ' ' || lex->source[end-1] == '\t'
           || lex->source[end-1] == '\n' || lex->source[end-1] == '\r')) {
        end--;
    }
    size_t len = end - start;
    const char *body = arena_strndup(lex->arena, lex->source + start, len);

    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.kind = TOK_ASM_BODY;
    tok.loc = loc;
    tok.text = body;
    tok.text_len = len;
    lex->asm_state = 0;
    return tok;
}

static Token lexer_next_raw(Lexer *lex);

static void update_asm_state(Lexer *lex, Token *tok) {
    if (tok->kind == TOK_ASM) {
        lex->asm_state = 1;
    } else if (lex->asm_state == 1 && tok->kind == TOK_IDENT) {
        lex->asm_state = 2;
    } else if (lex->asm_state == 2 && tok->kind == TOK_LBRACE) {
        lex->asm_state = 3;
    } else {
        lex->asm_state = 0;
    }
}

Token lexer_next(Lexer *lex)
{
    if (lex->has_peeked) {
        lex->has_peeked = false;
        Token tok = lex->peeked_token;
        update_asm_state(lex, &tok);
        return tok;
    }

    /* If we just saw asm <target> {, collect raw body */
    if (lex->asm_state == 3) {
        return lex_asm_body(lex);
    }

    Token tok = lexer_next_raw(lex);
    update_asm_state(lex, &tok);
    return tok;
}

static Token lexer_next_raw(Lexer *lex)
{
    for (;;) {
        skip_whitespace(lex);

        if (is_at_end(lex)) {
            Token tok;
            memset(&tok, 0, sizeof(tok));
            tok.kind     = TOK_EOF;
            tok.loc      = make_loc(lex);
            tok.text     = "";
            tok.text_len = 0;
            return tok;
        }

        char c = peek_char(lex);

        /* Comments */
        if (c == '/' && peek_char_at(lex, 1) == '/') {
            advance(lex); /* skip first  '/' */
            advance(lex); /* skip second '/' */
            skip_line_comment(lex);
            continue;     /* loop back */
        }
        if (c == '/' && peek_char_at(lex, 1) == '*') {
            advance(lex); /* skip '/' */
            advance(lex); /* skip '*' */
            skip_block_comment(lex);
            continue;     /* loop back */
        }

        /* Numbers */
        if (isdigit((unsigned char)c)) {
            return lex_number(lex);
        }

        /* Identifiers / keywords */
        if (isalpha((unsigned char)c) || c == '_') {
            return lex_identifier_or_keyword(lex);
        }

        /* Multi-char operators */
        switch (c) {
        case '=':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_EQUAL, 2);
            return make_token(lex, TOK_ASSIGN, 1);
        case '!':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_NOT_EQUAL, 2);
            return make_token(lex, TOK_BANG, 1);
        case '<':
            if (peek_char_at(lex, 1) == '<' && peek_char_at(lex, 2) == '=') return make_token(lex, TOK_SHL_ASSIGN, 3);
            if (peek_char_at(lex, 1) == '<') return make_token(lex, TOK_SHL, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_LESS_EQ, 2);
            return make_token(lex, TOK_LESS, 1);
        case '>':
            if (peek_char_at(lex, 1) == '>' && peek_char_at(lex, 2) == '=') return make_token(lex, TOK_SHR_ASSIGN, 3);
            if (peek_char_at(lex, 1) == '>') return make_token(lex, TOK_SHR, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_GREATER_EQ, 2);
            return make_token(lex, TOK_GREATER, 1);
        case '&':
            if (peek_char_at(lex, 1) == '&') return make_token(lex, TOK_AND, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_AMP_ASSIGN, 2);
            return make_token(lex, TOK_AMP, 1);
        case '|':
            if (peek_char_at(lex, 1) == '|') return make_token(lex, TOK_OR, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_PIPE_ASSIGN, 2);
            return make_token(lex, TOK_PIPE, 1);
        case '-':
            if (peek_char_at(lex, 1) == '>') return make_token(lex, TOK_ARROW, 2);
            if (peek_char_at(lex, 1) == '-') return make_token(lex, TOK_MINUS_MINUS, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_MINUS_ASSIGN, 2);
            return make_token(lex, TOK_MINUS, 1);

        /* Multi/single-char operators */
        case '+':
            if (peek_char_at(lex, 1) == '+') return make_token(lex, TOK_PLUS_PLUS, 2);
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_PLUS_ASSIGN, 2);
            return make_token(lex, TOK_PLUS, 1);
        case '*':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_STAR_ASSIGN, 2);
            return make_token(lex, TOK_STAR, 1);
        case '/':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_SLASH_ASSIGN, 2);
            return make_token(lex, TOK_SLASH, 1);
        case '%':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_PERCENT_ASSIGN, 2);
            return make_token(lex, TOK_PERCENT, 1);
        case '^':
            if (peek_char_at(lex, 1) == '=') return make_token(lex, TOK_CARET_ASSIGN, 2);
            return make_token(lex, TOK_CARET, 1);
        case '~': return make_token(lex, TOK_TILDE, 1);

        /* Punctuation */
        case '(': return make_token(lex, TOK_LPAREN, 1);
        case ')': return make_token(lex, TOK_RPAREN, 1);
        case '{': return make_token(lex, TOK_LBRACE, 1);
        case '}': return make_token(lex, TOK_RBRACE, 1);
        case '[': return make_token(lex, TOK_LBRACKET, 1);
        case ']': return make_token(lex, TOK_RBRACKET, 1);
        case ';': return make_token(lex, TOK_SEMICOLON, 1);
        case ',': return make_token(lex, TOK_COMMA, 1);
        case ':': return make_token(lex, TOK_COLON, 1);
        case '.': return make_token(lex, TOK_DOT, 1);

        case '\'': {
            /* Char literal: 'a' → int value 97 */
            Token tok;
            memset(&tok, 0, sizeof(tok));
            tok.loc = make_loc(lex);
            advance(lex); /* skip opening quote */
            char ch;
            if (peek_char(lex) == '\\') {
                advance(lex); /* skip backslash */
                char esc = peek_char(lex);
                advance(lex);
                switch (esc) {
                    case 'n':  ch = '\n'; break;
                    case 't':  ch = '\t'; break;
                    case 'r':  ch = '\r'; break;
                    case '0':  ch = '\0'; break;
                    case '\\': ch = '\\'; break;
                    case '\'': ch = '\''; break;
                    default:   ch = esc;  break;
                }
            } else {
                ch = peek_char(lex);
                advance(lex);
            }
            if (peek_char(lex) != '\'') {
                error_report(tok.loc, ERR_LEXER, "unterminated char literal");
                tok.kind = TOK_ERROR;
                tok.text = "";
                tok.text_len = 0;
                return tok;
            }
            advance(lex); /* skip closing quote */
            tok.kind = TOK_INT_LIT;
            char buf[2] = { ch, '\0' };
            tok.text = strtab_intern(lex->strtab, buf, 1);
            tok.text_len = 1;
            tok.int_value = (uint64_t)(unsigned char)ch;
            return tok;
        }

        case '"': {
            /* String literal with escape sequence support */
            Token tok;
            memset(&tok, 0, sizeof(tok));
            tok.loc = make_loc(lex);
            advance(lex); /* skip opening quote */

            /* First pass: compute output length */
            size_t raw_start = lex->pos;
            size_t raw_len = 0;
            size_t out_len = 0;
            while (!is_at_end(lex) && peek_char(lex) != '"') {
                if (peek_char(lex) == '\n') break;
                if (peek_char(lex) == '\\') { advance(lex); raw_len++; }
                advance(lex);
                raw_len++;
                out_len++;
            }
            if (is_at_end(lex) || peek_char(lex) != '"') {
                error_report(tok.loc, ERR_LEXER, "unterminated string literal");
                tok.kind = TOK_ERROR;
                tok.text = "";
                tok.text_len = 0;
                return tok;
            }
            advance(lex); /* skip closing quote */

            /* Second pass: build output with escapes processed */
            char *buf = arena_alloc(lex->arena, out_len + 1);
            const char *src = lex->source + raw_start;
            size_t j = 0;
            for (size_t i = 0; i < raw_len && j < out_len; ) {
                if (src[i] == '\\' && i + 1 < raw_len) {
                    i++;
                    switch (src[i]) {
                        case 'n':  buf[j++] = '\n'; break;
                        case 't':  buf[j++] = '\t'; break;
                        case 'r':  buf[j++] = '\r'; break;
                        case '0':  buf[j++] = '\0'; break;
                        case '\\': buf[j++] = '\\'; break;
                        case '"':  buf[j++] = '"';  break;
                        default:   buf[j++] = src[i]; break;
                    }
                    i++;
                } else {
                    buf[j++] = src[i++];
                }
            }
            buf[j] = '\0';

            tok.kind = TOK_STRING_LIT;
            tok.text = buf;
            tok.text_len = j;
            return tok;
        }

        default: {
            Token tok;
            memset(&tok, 0, sizeof(tok));
            tok.kind = TOK_ERROR;
            tok.loc  = make_loc(lex);
            const char *s = lex->source + lex->pos;
            tok.text     = strtab_intern(lex->strtab, s, 1);
            tok.text_len = 1;
            error_report(tok.loc, ERR_LEXER, "unexpected character '%c'", c);
            advance(lex);
            return tok;
        }
        }
    }
}

Token lexer_peek(Lexer *lex)
{
    if (!lex->has_peeked) {
        lex->peeked_token = lexer_next(lex);
        lex->has_peeked   = true;
    }
    return lex->peeked_token;
}

int lexer_lex_all(Lexer *lex, Token **out)
{
    int capacity = 256;
    int count    = 0;
    Token *tokens = (Token *)arena_alloc(lex->arena, (size_t)capacity * sizeof(Token));

    for (;;) {
        if (count >= capacity) {
            int new_cap    = capacity * 2;
            Token *bigger  = (Token *)arena_alloc(lex->arena, (size_t)new_cap * sizeof(Token));
            memcpy(bigger, tokens, (size_t)count * sizeof(Token));
            tokens   = bigger;
            capacity = new_cap;
        }

        tokens[count] = lexer_next(lex);
        if (tokens[count].kind == TOK_EOF) {
            count++;
            break;
        }
        count++;
    }

    *out = tokens;
    return count;
}
