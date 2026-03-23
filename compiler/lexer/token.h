#ifndef HPP_TOKEN_H
#define HPP_TOKEN_H

#include "common/error.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    /* Literals */
    TOK_INT_LIT, TOK_BOOL_LIT, TOK_STRING_LIT,
    /* Keywords (18 active) */
    TOK_FN, TOK_RETURN, TOK_DEF, TOK_IF, TOK_ELSE, TOK_FOR, TOK_WHILE,
    TOK_BREAK, TOK_CONTINUE, TOK_CONST, TOK_LET, TOK_ASM, TOK_OPP, TOK_SWITCH, TOK_CASE, TOK_DEFAULT,
    TOK_TRUE, TOK_FALSE, TOK_NULL, TOK_BIT,
    TOK_IMPORT, TOK_LINK, TOK_MACRO, TOK_DEFX,
    /* Reserved future */
    TOK_STRUCT, TOK_ENUM, TOK_MATCH, TOK_AS,
    /* Identifier */
    TOK_IDENT,
    /* Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_ASSIGN, TOK_EQUAL, TOK_NOT_EQUAL,
    TOK_LESS, TOK_GREATER, TOK_LESS_EQ, TOK_GREATER_EQ,
    TOK_AMP, TOK_PIPE, TOK_CARET, TOK_TILDE, TOK_BANG,
    TOK_SHL, TOK_SHR, TOK_AND, TOK_OR,
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN,
    TOK_PERCENT_ASSIGN, TOK_AMP_ASSIGN, TOK_PIPE_ASSIGN, TOK_CARET_ASSIGN,
    TOK_SHL_ASSIGN, TOK_SHR_ASSIGN,
    TOK_PLUS_PLUS, TOK_MINUS_MINUS,
    /* Punctuation */
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMICOLON, TOK_COMMA, TOK_COLON, TOK_DOT, TOK_ARROW,
    /* Special */
    TOK_ASM_BODY,  /* raw assembly text between { } in asm blocks */
    TOK_EOF, TOK_ERROR,
} TokenKind;

typedef struct {
    TokenKind    kind;
    SourceLoc    loc;
    const char  *text;
    size_t       text_len;
    uint64_t     int_value;
    bool         bool_value;
} Token;

const char *token_kind_str(TokenKind kind);
void        token_print(const Token *tok);

#endif
