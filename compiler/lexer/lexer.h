#ifndef HPP_LEXER_H
#define HPP_LEXER_H

#include "lexer/token.h"
#include "common/arena.h"
#include "common/strtab.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *source;
    size_t      source_len;
    const char *filename;
    size_t      pos;
    int         line;
    int         col;
    size_t      line_start;
    Arena      *arena;
    StrTab     *strtab;
    bool        has_peeked;
    Token       peeked_token;
    int         asm_state;  /* 0=normal, 1=saw asm, 2=saw target, 3=collect body */
} Lexer;

void  lexer_init(Lexer *lex, const char *source, size_t len,
                 const char *filename, Arena *arena, StrTab *strtab);
Token lexer_next(Lexer *lex);
Token lexer_peek(Lexer *lex);
int   lexer_lex_all(Lexer *lex, Token **out);

#endif
