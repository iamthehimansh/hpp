#include "lexer/token.h"
#include <stdio.h>
#include <inttypes.h>

const char *token_kind_str(TokenKind kind)
{
    switch (kind) {
    case TOK_INT_LIT:     return "INT_LIT";
    case TOK_BOOL_LIT:    return "BOOL_LIT";
    case TOK_FN:          return "FN";
    case TOK_RETURN:      return "RETURN";
    case TOK_DEF:         return "DEF";
    case TOK_IF:          return "IF";
    case TOK_ELSE:        return "ELSE";
    case TOK_FOR:         return "FOR";
    case TOK_WHILE:       return "WHILE";
    case TOK_BREAK:       return "BREAK";
    case TOK_CONTINUE:    return "CONTINUE";
    case TOK_CONST:       return "CONST";
    case TOK_LET:         return "LET";
    case TOK_ASM:         return "ASM";
    case TOK_OPP:         return "OPP";
    case TOK_SWITCH:      return "SWITCH";
    case TOK_CASE:        return "CASE";
    case TOK_DEFAULT:     return "DEFAULT";
    case TOK_TRUE:        return "TRUE";
    case TOK_FALSE:       return "FALSE";
    case TOK_NULL:        return "NULL";
    case TOK_BIT:         return "BIT";
    case TOK_STRING_LIT:  return "STRING_LIT";
    case TOK_IMPORT:      return "IMPORT";
    case TOK_LINK:        return "LINK";
    case TOK_MACRO:       return "MACRO";
    case TOK_DEFX:        return "DEFX";
    case TOK_STRUCT:      return "STRUCT";
    case TOK_ENUM:        return "ENUM";
    case TOK_MATCH:       return "MATCH";
    case TOK_AS:          return "AS";
    case TOK_IDENT:       return "IDENT";
    case TOK_PLUS:        return "PLUS";
    case TOK_MINUS:       return "MINUS";
    case TOK_STAR:        return "STAR";
    case TOK_SLASH:       return "SLASH";
    case TOK_PERCENT:     return "PERCENT";
    case TOK_ASSIGN:      return "ASSIGN";
    case TOK_EQUAL:       return "EQUAL";
    case TOK_NOT_EQUAL:   return "NOT_EQUAL";
    case TOK_LESS:        return "LESS";
    case TOK_GREATER:     return "GREATER";
    case TOK_LESS_EQ:     return "LESS_EQ";
    case TOK_GREATER_EQ:  return "GREATER_EQ";
    case TOK_AMP:         return "AMP";
    case TOK_PIPE:        return "PIPE";
    case TOK_CARET:       return "CARET";
    case TOK_TILDE:       return "TILDE";
    case TOK_BANG:        return "BANG";
    case TOK_SHL:         return "SHL";
    case TOK_SHR:         return "SHR";
    case TOK_AND:         return "AND";
    case TOK_OR:          return "OR";
    case TOK_PLUS_ASSIGN: return "PLUS_ASSIGN";
    case TOK_MINUS_ASSIGN:return "MINUS_ASSIGN";
    case TOK_STAR_ASSIGN: return "STAR_ASSIGN";
    case TOK_SLASH_ASSIGN:return "SLASH_ASSIGN";
    case TOK_PERCENT_ASSIGN:return "PERCENT_ASSIGN";
    case TOK_AMP_ASSIGN:  return "AMP_ASSIGN";
    case TOK_PIPE_ASSIGN: return "PIPE_ASSIGN";
    case TOK_CARET_ASSIGN:return "CARET_ASSIGN";
    case TOK_SHL_ASSIGN:  return "SHL_ASSIGN";
    case TOK_SHR_ASSIGN:  return "SHR_ASSIGN";
    case TOK_PLUS_PLUS:   return "PLUS_PLUS";
    case TOK_MINUS_MINUS: return "MINUS_MINUS";
    case TOK_LPAREN:      return "LPAREN";
    case TOK_RPAREN:      return "RPAREN";
    case TOK_LBRACE:      return "LBRACE";
    case TOK_RBRACE:      return "RBRACE";
    case TOK_LBRACKET:    return "LBRACKET";
    case TOK_RBRACKET:    return "RBRACKET";
    case TOK_SEMICOLON:   return "SEMICOLON";
    case TOK_COMMA:       return "COMMA";
    case TOK_COLON:       return "COLON";
    case TOK_DOT:         return "DOT";
    case TOK_ARROW:       return "ARROW";
    case TOK_ASM_BODY:    return "ASM_BODY";
    case TOK_EOF:         return "EOF";
    case TOK_ERROR:       return "ERROR";
    }
    return "UNKNOWN";
}

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
