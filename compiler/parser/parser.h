#ifndef HPP_PARSER_H
#define HPP_PARSER_H

#include "common/arena.h"
#include "common/strtab.h"
#include "lexer/token.h"
#include "ast/ast.h"

#define MAX_TYPE_NAMES 256

typedef struct {
    Token *tokens;
    int token_count;
    int pos;
    Arena *arena;
    StrTab *strtab;
    const char *filename;
    bool had_error;
    bool panic_mode;
    /* Track known type names for disambiguation */
    const char *type_names[MAX_TYPE_NAMES];
    int type_name_count;
} Parser;

void     parser_init(Parser *p, Token *tokens, int count,
                     Arena *arena, StrTab *strtab, const char *filename);
AstNode *parser_parse(Parser *p);

#endif
