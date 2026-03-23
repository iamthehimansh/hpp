/* token_print stays in C (needs printf).
   token_kind_str is provided by the H++ module. */
#include "lexer/token.h"
#include <stdio.h>
#include <inttypes.h>

void token_print(const Token *tok)
{
    printf("%d:%d %s '%.*s'",
           tok->loc.line, tok->loc.col,
           token_kind_str(tok->kind),
           (int)tok->text_len, tok->text ? tok->text : "");

    if (tok->kind == TOK_INT_LIT) {
        printf(" value=%" PRIu64, tok->int_value);
    } else if (tok->kind == TOK_BOOL_LIT) {
        printf(" value=%s", tok->bool_value ? "true" : "false");
    }

    printf("\n");
}
