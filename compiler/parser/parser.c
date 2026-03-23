#include "parser/parser.h"
#include "common/error.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/*  Forward declarations                                               */
/* ------------------------------------------------------------------ */
static AstNode *parse_program(Parser *p);
static AstNode *parse_declaration(Parser *p);
static AstNode *parse_type_def(Parser *p);
static AstNode *parse_function_def(Parser *p);
static AstNode *parse_opp_def(Parser *p);
static AstNode *parse_import(Parser *p);
static AstNode *parse_link(Parser *p);
static AstNode *parse_type(Parser *p);
static AstNode *parse_block(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_var_decl(Parser *p, DeclKind kind);
static AstNode *parse_return_stmt(Parser *p);
static AstNode *parse_if_stmt(Parser *p);
static AstNode *parse_while_stmt(Parser *p);
static AstNode *parse_for_stmt(Parser *p);
static AstNode *parse_asm_block(Parser *p);
static AstNode *parse_expression(Parser *p);
static AstNode *parse_assignment(Parser *p);
static AstNode *parse_logic_or(Parser *p);
static AstNode *parse_logic_and(Parser *p);
static AstNode *parse_bit_or(Parser *p);
static AstNode *parse_bit_xor(Parser *p);
static AstNode *parse_bit_and(Parser *p);
static AstNode *parse_equality(Parser *p);
static AstNode *parse_comparison(Parser *p);
static AstNode *parse_shift(Parser *p);
static AstNode *parse_additive(Parser *p);
static AstNode *parse_multiplicative(Parser *p);
static AstNode *parse_unary(Parser *p);
static AstNode *parse_call(Parser *p);
static AstNode *parse_primary(Parser *p);
static AstNode *parse_cast(Parser *p, AstNode *type_node);
static AstNode *parse_var_decl_no_semi(Parser *p, DeclKind kind);

/* ------------------------------------------------------------------ */
/*  Token helpers                                                      */
/* ------------------------------------------------------------------ */
static Token eof_sentinel;

static Token *current(Parser *p)
{
    if (p->pos >= p->token_count) {
        return &eof_sentinel;
    }
    return &p->tokens[p->pos];
}

static Token *previous(Parser *p)
{
    return &p->tokens[p->pos - 1];
}

static Token *advance(Parser *p)
{
    if (current(p)->kind != TOK_EOF) {
        p->pos++;
    }
    return previous(p);
}

static bool check(Parser *p, TokenKind kind)
{
    return current(p)->kind == kind;
}

static bool match(Parser *p, TokenKind kind)
{
    if (check(p, kind)) {
        advance(p);
        return true;
    }
    return false;
}

static TokenKind peek_kind(Parser *p)
{
    return current(p)->kind;
}

static Token *expect(Parser *p, TokenKind kind, const char *msg);

/* ------------------------------------------------------------------ */
/*  Type name tracking                                                 */
/* ------------------------------------------------------------------ */
static bool is_type_name(Parser *p, const char *name)
{
    for (int i = 0; i < p->type_name_count; i++) {
        if (p->type_names[i] == name) {
            return true;
        }
    }
    return false;
}

static void register_type_name(Parser *p, const char *name)
{
    if (is_type_name(p, name)) {
        return;
    }
    if (p->type_name_count < MAX_TYPE_NAMES) {
        p->type_names[p->type_name_count++] = name;
    }
}

/* ------------------------------------------------------------------ */
/*  Error handling                                                     */
/* ------------------------------------------------------------------ */
static void parser_error(Parser *p, Token *tok, const char *msg)
{
    if (p->panic_mode) {
        return;
    }
    p->panic_mode = true;
    p->had_error = true;
    error_report(tok->loc, ERR_PARSER, "%s (got '%s')", msg,
                 token_kind_str(tok->kind));
}

static void synchronize(Parser *p)
{
    p->panic_mode = false;
    while (current(p)->kind != TOK_EOF) {
        if (previous(p)->kind == TOK_SEMICOLON) {
            return;
        }
        switch (current(p)->kind) {
        case TOK_FN:
        case TOK_DEF:
        case TOK_IF:
        case TOK_WHILE:
        case TOK_FOR:
        case TOK_RETURN:
        case TOK_LET:
        case TOK_CONST:
        case TOK_OPP:
        case TOK_RBRACE:
            return;
        default:
            break;
        }
        advance(p);
    }
}

static Token *expect(Parser *p, TokenKind kind, const char *msg)
{
    if (check(p, kind)) {
        return advance(p);
    }
    parser_error(p, current(p), msg);
    return current(p);
}

/* ------------------------------------------------------------------ */
/*  Public interface                                                   */
/* ------------------------------------------------------------------ */
void parser_init(Parser *p, Token *tokens, int count,
                 Arena *arena, StrTab *strtab, const char *filename)
{
    memset(p, 0, sizeof(Parser));
    p->tokens = tokens;
    p->token_count = count;
    p->pos = 0;
    p->arena = arena;
    p->strtab = strtab;
    p->filename = filename;
    p->had_error = false;
    p->panic_mode = false;
    p->type_name_count = 0;

    /* Initialise the EOF sentinel once */
    eof_sentinel.kind = TOK_EOF;
    eof_sentinel.loc.filename = filename;
    eof_sentinel.loc.line = 0;
    eof_sentinel.loc.col = 0;
    eof_sentinel.text = "";
    eof_sentinel.text_len = 0;
    eof_sentinel.int_value = 0;
    eof_sentinel.bool_value = false;
}

AstNode *parser_parse(Parser *p)
{
    return parse_program(p);
}

/* ------------------------------------------------------------------ */
/*  Top-level                                                          */
/* ------------------------------------------------------------------ */
static AstNode *parse_program(Parser *p)
{
    AstNode *prog = ast_new(p->arena, NODE_PROGRAM, current(p)->loc);
    prog->as.program.decls = NULL;

    while (!check(p, TOK_EOF)) {
        AstNode *decl = parse_declaration(p);
        if (decl) {
            prog->as.program.decls =
                ast_list_append(p->arena, prog->as.program.decls, decl);
        }
        if (p->panic_mode) {
            synchronize(p);
        }
    }
    return prog;
}

static AstNode *parse_declaration(Parser *p)
{
    if (check(p, TOK_DEF))    return parse_type_def(p);
    if (check(p, TOK_FN))     return parse_function_def(p);
    if (check(p, TOK_OPP))    return parse_opp_def(p);
    if (check(p, TOK_IMPORT)) return parse_import(p);
    if (check(p, TOK_LINK))   return parse_link(p);
    return parse_statement(p);
}

/* ------------------------------------------------------------------ */
/*  def IDENT bit [ INT_LIT ] ;                                        */
/* ------------------------------------------------------------------ */
static AstNode *parse_type_def(Parser *p)
{
    Token *def_tok = advance(p);  /* consume 'def' */
    Token *name_tok = expect(p, TOK_IDENT, "expected type name after 'def'");
    const char *name = strtab_intern(p->strtab, name_tok->text, name_tok->text_len);

    expect(p, TOK_BIT, "expected 'bit' in type definition");
    expect(p, TOK_LBRACKET, "expected '[' after 'bit'");
    Token *width_tok = expect(p, TOK_INT_LIT, "expected integer literal for bit width");
    uint32_t width = (uint32_t)width_tok->int_value;
    expect(p, TOK_RBRACKET, "expected ']' after bit width");
    expect(p, TOK_SEMICOLON, "expected ';' after type definition");

    register_type_name(p, name);

    AstNode *node = ast_new(p->arena, NODE_TYPE_DEF, def_tok->loc);
    node->as.type_def.name = name;
    node->as.type_def.bit_width = width;
    return node;
}

/* ------------------------------------------------------------------ */
/*  fn IDENT ( params? ) ( -> type )? block                            */
/* ------------------------------------------------------------------ */
static AstNode *parse_function_def(Parser *p)
{
    Token *fn_tok = advance(p);  /* consume 'fn' */
    Token *name_tok = expect(p, TOK_IDENT, "expected function name after 'fn'");
    const char *name = strtab_intern(p->strtab, name_tok->text, name_tok->text_len);

    expect(p, TOK_LPAREN, "expected '(' after function name");

    /* Parse parameter list */
    Param *params = NULL;
    int param_count = 0;
    int param_cap = 0;

    if (!check(p, TOK_RPAREN)) {
        do {
            Token *pname_tok = expect(p, TOK_IDENT, "expected parameter name");
            const char *pname = strtab_intern(p->strtab, pname_tok->text, pname_tok->text_len);
            expect(p, TOK_COLON, "expected ':' after parameter name");
            AstNode *ptype = parse_type(p);

            if (param_count >= param_cap) {
                int new_cap = param_cap == 0 ? 8 : param_cap * 2;
                Param *new_params = (Param *)arena_alloc(p->arena, sizeof(Param) * (size_t)new_cap);
                if (params) {
                    memcpy(new_params, params, sizeof(Param) * (size_t)param_count);
                }
                params = new_params;
                param_cap = new_cap;
            }
            params[param_count].name = pname;
            params[param_count].type_node = ptype;
            params[param_count].loc = pname_tok->loc;
            param_count++;
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, "expected ')' after parameter list");

    /* Optional return type */
    AstNode *ret_type = NULL;
    if (match(p, TOK_ARROW)) {
        ret_type = parse_type(p);
    }

    /* Function declaration (;) or definition ({body}) */
    AstNode *body = NULL;
    if (check(p, TOK_LBRACE)) {
        body = parse_block(p);
    } else {
        /* Declaration only (no body) — used in .hdef files */
        expect(p, TOK_SEMICOLON, "expected '{' or ';' after function signature");
    }

    AstNode *node = ast_new(p->arena, NODE_FUNC_DEF, fn_tok->loc);
    node->as.func_def.name = name;
    node->as.func_def.params.params = params;
    node->as.func_def.params.count = param_count;
    node->as.func_def.return_type = ret_type;
    node->as.func_def.body = body;
    return node;
}

/* ------------------------------------------------------------------ */
/*  opp type OP type -> type { asm_block+ }                            */
/*  opp OP type -> type { asm_block+ }   (unary)                       */
/* ------------------------------------------------------------------ */
static BinOp token_to_binop(TokenKind kind)
{
    switch (kind) {
    case TOK_PLUS:       return BINOP_ADD;
    case TOK_MINUS:      return BINOP_SUB;
    case TOK_STAR:       return BINOP_MUL;
    case TOK_SLASH:      return BINOP_DIV;
    case TOK_PERCENT:    return BINOP_MOD;
    case TOK_AMP:        return BINOP_BIT_AND;
    case TOK_PIPE:       return BINOP_BIT_OR;
    case TOK_CARET:      return BINOP_BIT_XOR;
    case TOK_SHL:        return BINOP_SHL;
    case TOK_SHR:        return BINOP_SHR;
    case TOK_EQUAL:      return BINOP_EQ;
    case TOK_NOT_EQUAL:  return BINOP_NE;
    case TOK_LESS:       return BINOP_LT;
    case TOK_GREATER:    return BINOP_GT;
    case TOK_LESS_EQ:    return BINOP_LE;
    case TOK_GREATER_EQ: return BINOP_GE;
    case TOK_AND:        return BINOP_LOGIC_AND;
    case TOK_OR:         return BINOP_LOGIC_OR;
    default:             return BINOP_ADD; /* fallback */
    }
}

static bool is_operator_token(TokenKind kind)
{
    switch (kind) {
    case TOK_PLUS: case TOK_MINUS: case TOK_STAR: case TOK_SLASH:
    case TOK_PERCENT: case TOK_AMP: case TOK_PIPE: case TOK_CARET:
    case TOK_SHL: case TOK_SHR: case TOK_EQUAL: case TOK_NOT_EQUAL:
    case TOK_LESS: case TOK_GREATER: case TOK_LESS_EQ: case TOK_GREATER_EQ:
    case TOK_AND: case TOK_OR: case TOK_BANG: case TOK_TILDE:
        return true;
    default:
        return false;
    }
}

static UnOp token_to_unop(TokenKind kind)
{
    switch (kind) {
    case TOK_MINUS: return UNOP_NEG;
    case TOK_BANG:  return UNOP_NOT;
    case TOK_TILDE: return UNOP_BITNOT;
    default:        return UNOP_NEG; /* fallback */
    }
}

static AstNode *parse_opp_def(Parser *p)
{
    Token *opp_tok = advance(p);  /* consume 'opp' */

    AstNode *node = ast_new(p->arena, NODE_OPP_DEF, opp_tok->loc);
    node->as.opp_def.asm_blocks = NULL;

    /* Check if unary: next token is an operator (not a type name) */
    if (is_operator_token(peek_kind(p))) {
        /* Unary: opp OP type -> type { ... } */
        Token *op_tok = advance(p);
        node->as.opp_def.is_unary = true;
        node->as.opp_def.unary_op = token_to_unop(op_tok->kind);
        node->as.opp_def.op = BINOP_ADD; /* unused for unary */
        node->as.opp_def.left_type = NULL;
        node->as.opp_def.right_type = parse_type(p);
    } else {
        /* Binary: opp type OP type -> type { ... } */
        node->as.opp_def.is_unary = false;
        node->as.opp_def.left_type = parse_type(p);

        Token *op_tok = advance(p);
        if (!is_operator_token(op_tok->kind)) {
            parser_error(p, op_tok, "expected operator in 'opp' definition");
        }
        node->as.opp_def.op = token_to_binop(op_tok->kind);
        node->as.opp_def.unary_op = UNOP_NEG; /* unused for binary */
        node->as.opp_def.right_type = parse_type(p);
    }

    expect(p, TOK_ARROW, "expected '->' after types in 'opp' definition");
    node->as.opp_def.result_type = parse_type(p);

    expect(p, TOK_LBRACE, "expected '{' in 'opp' definition");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *ab = parse_asm_block(p);
        if (ab) {
            node->as.opp_def.asm_blocks =
                ast_list_append(p->arena, node->as.opp_def.asm_blocks, ab);
        }
        if (p->panic_mode) {
            synchronize(p);
        }
    }
    expect(p, TOK_RBRACE, "expected '}' after 'opp' body");
    return node;
}

/* ------------------------------------------------------------------ */
/*  import / link                                                      */
/* ------------------------------------------------------------------ */

/* import std;
   import std/io;
   import std/{io, mem, str};                                          */
static AstNode *parse_import(Parser *p)
{
    Token *imp_tok = advance(p); /* consume 'import' */

    /* Collect path segments: ident ( '/' ident )* */
    const char *segments[32];
    int seg_count = 0;

    Token *first = expect(p, TOK_IDENT, "expected module name after 'import'");
    segments[seg_count++] = first->text;

    const char *names[64];
    int name_count = 0;

    while (match(p, TOK_SLASH)) {
        if (check(p, TOK_LBRACE)) {
            /* Multi-import: import std/{io, mem, str} */
            advance(p); /* consume '{' */
            do {
                Token *nm = expect(p, TOK_IDENT, "expected module name in import list");
                if (name_count < 64) names[name_count++] = nm->text;
            } while (match(p, TOK_COMMA));
            expect(p, TOK_RBRACE, "expected '}' after import list");
            break;
        }
        Token *seg = expect(p, TOK_IDENT, "expected module name after '/'");
        if (seg_count < 32) segments[seg_count++] = seg->text;
    }

    expect(p, TOK_SEMICOLON, "expected ';' after import");

    AstNode *node = ast_new(p->arena, NODE_IMPORT, imp_tok->loc);
    node->as.import_decl.segments = arena_alloc(p->arena,
        (size_t)seg_count * sizeof(const char *));
    for (int i = 0; i < seg_count; i++)
        node->as.import_decl.segments[i] = segments[i];
    node->as.import_decl.segment_count = seg_count;

    if (name_count > 0) {
        node->as.import_decl.names = arena_alloc(p->arena,
            (size_t)name_count * sizeof(const char *));
        for (int i = 0; i < name_count; i++)
            node->as.import_decl.names[i] = names[i];
        node->as.import_decl.name_count = name_count;
    } else {
        node->as.import_decl.names = NULL;
        node->as.import_decl.name_count = 0;
    }

    return node;
}

/* link "path/to/lib.o"; */
static AstNode *parse_link(Parser *p)
{
    Token *link_tok = advance(p); /* consume 'link' */
    Token *path = expect(p, TOK_STRING_LIT, "expected string path after 'link'");
    expect(p, TOK_SEMICOLON, "expected ';' after link path");

    AstNode *node = ast_new(p->arena, NODE_LINK, link_tok->loc);
    node->as.link_decl.path = path->text;
    return node;
}

/* ------------------------------------------------------------------ */
/*  Type parsing                                                       */
/* ------------------------------------------------------------------ */
static AstNode *parse_type(Parser *p)
{
    if (match(p, TOK_BIT)) {
        Token *bit_tok = previous(p);
        uint32_t width = 0;  /* 0 means plain 'bit' = bit[1] */
        if (match(p, TOK_LBRACKET)) {
            Token *w = expect(p, TOK_INT_LIT, "expected integer for bit width");
            width = (uint32_t)w->int_value;
            expect(p, TOK_RBRACKET, "expected ']' after bit width");
        }
        AstNode *node = ast_new(p->arena, NODE_TYPE_BIT, bit_tok->loc);
        node->as.type_bit.width = width;
        return node;
    }
    if (check(p, TOK_IDENT)) {
        Token *name_tok = advance(p);
        const char *name = strtab_intern(p->strtab, name_tok->text, name_tok->text_len);
        AstNode *node = ast_new(p->arena, NODE_TYPE_NAMED, name_tok->loc);
        node->as.type_named.name = name;
        return node;
    }
    parser_error(p, current(p), "expected type");
    return ast_new(p->arena, NODE_TYPE_BIT, current(p)->loc);
}

/* ------------------------------------------------------------------ */
/*  Block                                                              */
/* ------------------------------------------------------------------ */
static AstNode *parse_block(Parser *p)
{
    Token *brace = expect(p, TOK_LBRACE, "expected '{'");
    AstNode *node = ast_new(p->arena, NODE_BLOCK, brace->loc);
    node->as.block.stmts = NULL;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *stmt = parse_statement(p);
        if (stmt) {
            node->as.block.stmts =
                ast_list_append(p->arena, node->as.block.stmts, stmt);
        }
        if (p->panic_mode) {
            synchronize(p);
        }
    }
    expect(p, TOK_RBRACE, "expected '}'");
    return node;
}

/* ------------------------------------------------------------------ */
/*  Statement                                                          */
/* ------------------------------------------------------------------ */
static AstNode *parse_statement(Parser *p)
{
    switch (peek_kind(p)) {
    case TOK_RETURN:   return parse_return_stmt(p);
    case TOK_IF:       return parse_if_stmt(p);
    case TOK_WHILE:    return parse_while_stmt(p);
    case TOK_FOR:      return parse_for_stmt(p);
    case TOK_BREAK: {
        Token *tok = advance(p);
        expect(p, TOK_SEMICOLON, "expected ';' after 'break'");
        return ast_new(p->arena, NODE_BREAK_STMT, tok->loc);
    }
    case TOK_CONTINUE: {
        Token *tok = advance(p);
        expect(p, TOK_SEMICOLON, "expected ';' after 'continue'");
        return ast_new(p->arena, NODE_CONTINUE_STMT, tok->loc);
    }
    case TOK_ASM:    return parse_asm_block(p);
    case TOK_LBRACE: return parse_block(p);
    case TOK_CONST:  return parse_var_decl(p, DECL_CONST);
    case TOK_LET:    return parse_var_decl(p, DECL_LET);
    case TOK_BIT:    return parse_var_decl(p, DECL_TYPED);
    default:
        break;
    }

    /* Check for typed variable declaration: type_name IDENT */
    if (check(p, TOK_IDENT) &&
        is_type_name(p, strtab_intern(p->strtab, current(p)->text, current(p)->text_len)) &&
        p->pos + 1 < p->token_count &&
        p->tokens[p->pos + 1].kind == TOK_IDENT) {
        return parse_var_decl(p, DECL_TYPED);
    }

    /* Expression statement */
    {
        AstNode *expr = parse_expression(p);
        expect(p, TOK_SEMICOLON, "expected ';' after expression");
        AstNode *stmt = ast_new(p->arena, NODE_EXPR_STMT, expr->loc);
        stmt->as.expr_stmt.expr = expr;
        return stmt;
    }
}

/* ------------------------------------------------------------------ */
/*  Variable declaration                                               */
/* ------------------------------------------------------------------ */
static AstNode *parse_var_decl(Parser *p, DeclKind kind)
{
    SourceLoc loc = current(p)->loc;
    AstNode *type_node = NULL;
    const char *name = NULL;
    AstNode *init = NULL;

    switch (kind) {
    case DECL_CONST: {
        advance(p);  /* consume 'const' */
        Token *n = expect(p, TOK_IDENT, "expected variable name after 'const'");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        expect(p, TOK_ASSIGN, "expected '=' after const variable name");
        init = parse_expression(p);
        expect(p, TOK_SEMICOLON, "expected ';' after variable declaration");
        break;
    }
    case DECL_LET: {
        advance(p);  /* consume 'let' */
        Token *n = expect(p, TOK_IDENT, "expected variable name after 'let'");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        expect(p, TOK_ASSIGN, "expected '=' after let variable name");
        init = parse_expression(p);
        expect(p, TOK_SEMICOLON, "expected ';' after variable declaration");
        break;
    }
    case DECL_TYPED: {
        type_node = parse_type(p);
        Token *n = expect(p, TOK_IDENT, "expected variable name after type");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        if (match(p, TOK_ASSIGN)) {
            init = parse_expression(p);
        }
        expect(p, TOK_SEMICOLON, "expected ';' after variable declaration");
        break;
    }
    }

    AstNode *node = ast_new(p->arena, NODE_VAR_DECL, loc);
    node->as.var_decl.decl_kind = kind;
    node->as.var_decl.name = name;
    node->as.var_decl.type_node = type_node;
    node->as.var_decl.init_expr = init;
    return node;
}

/* ------------------------------------------------------------------ */
/*  return expr? ;                                                     */
/* ------------------------------------------------------------------ */
static AstNode *parse_return_stmt(Parser *p)
{
    Token *ret_tok = advance(p);  /* consume 'return' */
    AstNode *expr = NULL;
    if (!check(p, TOK_SEMICOLON)) {
        expr = parse_expression(p);
    }
    expect(p, TOK_SEMICOLON, "expected ';' after return statement");

    AstNode *node = ast_new(p->arena, NODE_RETURN_STMT, ret_tok->loc);
    node->as.return_stmt.expr = expr;
    return node;
}

/* ------------------------------------------------------------------ */
/*  if ( expr ) block ( else (if_stmt | block) )?                      */
/* ------------------------------------------------------------------ */
static AstNode *parse_if_stmt(Parser *p)
{
    Token *if_tok = advance(p);  /* consume 'if' */
    expect(p, TOK_LPAREN, "expected '(' after 'if'");
    AstNode *cond = parse_expression(p);
    expect(p, TOK_RPAREN, "expected ')' after if condition");
    AstNode *then_block = parse_block(p);
    AstNode *else_block = NULL;

    if (match(p, TOK_ELSE)) {
        if (check(p, TOK_IF)) {
            else_block = parse_if_stmt(p);
        } else {
            else_block = parse_block(p);
        }
    }

    AstNode *node = ast_new(p->arena, NODE_IF_STMT, if_tok->loc);
    node->as.if_stmt.condition = cond;
    node->as.if_stmt.then_block = then_block;
    node->as.if_stmt.else_block = else_block;
    return node;
}

/* ------------------------------------------------------------------ */
/*  while ( expr ) block                                               */
/* ------------------------------------------------------------------ */
static AstNode *parse_while_stmt(Parser *p)
{
    Token *while_tok = advance(p);  /* consume 'while' */
    expect(p, TOK_LPAREN, "expected '(' after 'while'");
    AstNode *cond = parse_expression(p);
    expect(p, TOK_RPAREN, "expected ')' after while condition");
    AstNode *body = parse_block(p);

    AstNode *node = ast_new(p->arena, NODE_WHILE_STMT, while_tok->loc);
    node->as.while_stmt.condition = cond;
    node->as.while_stmt.body = body;
    return node;
}

/* ------------------------------------------------------------------ */
/*  for ( init ; cond ; update ) block                                 */
/* ------------------------------------------------------------------ */
static AstNode *parse_for_stmt(Parser *p)
{
    Token *for_tok = advance(p);  /* consume 'for' */
    expect(p, TOK_LPAREN, "expected '(' after 'for'");

    /* init */
    AstNode *init = NULL;
    if (!check(p, TOK_SEMICOLON)) {
        if (check(p, TOK_CONST)) {
            init = parse_var_decl_no_semi(p, DECL_CONST);
        } else if (check(p, TOK_LET)) {
            init = parse_var_decl_no_semi(p, DECL_LET);
        } else if (check(p, TOK_BIT)) {
            init = parse_var_decl_no_semi(p, DECL_TYPED);
        } else if (check(p, TOK_IDENT) &&
                   is_type_name(p, strtab_intern(p->strtab, current(p)->text, current(p)->text_len)) &&
                   p->pos + 1 < p->token_count &&
                   p->tokens[p->pos + 1].kind == TOK_IDENT) {
            init = parse_var_decl_no_semi(p, DECL_TYPED);
        } else {
            init = parse_expression(p);
        }
    }
    expect(p, TOK_SEMICOLON, "expected ';' after for-init");

    /* condition */
    AstNode *cond = NULL;
    if (!check(p, TOK_SEMICOLON)) {
        cond = parse_expression(p);
    }
    expect(p, TOK_SEMICOLON, "expected ';' after for-condition");

    /* update */
    AstNode *update = NULL;
    if (!check(p, TOK_RPAREN)) {
        update = parse_expression(p);
    }
    expect(p, TOK_RPAREN, "expected ')' after for-update");

    AstNode *body = parse_block(p);

    AstNode *node = ast_new(p->arena, NODE_FOR_STMT, for_tok->loc);
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = cond;
    node->as.for_stmt.update = update;
    node->as.for_stmt.body = body;
    return node;
}

/* ------------------------------------------------------------------ */
/*  Variable decl without trailing semicolon (for for-init)            */
/* ------------------------------------------------------------------ */
static AstNode *parse_var_decl_no_semi(Parser *p, DeclKind kind)
{
    SourceLoc loc = current(p)->loc;
    AstNode *type_node = NULL;
    const char *name = NULL;
    AstNode *init_expr = NULL;

    switch (kind) {
    case DECL_CONST: {
        advance(p);
        Token *n = expect(p, TOK_IDENT, "expected variable name after 'const'");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        expect(p, TOK_ASSIGN, "expected '=' after const variable name");
        init_expr = parse_expression(p);
        break;
    }
    case DECL_LET: {
        advance(p);
        Token *n = expect(p, TOK_IDENT, "expected variable name after 'let'");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        expect(p, TOK_ASSIGN, "expected '=' after let variable name");
        init_expr = parse_expression(p);
        break;
    }
    case DECL_TYPED: {
        type_node = parse_type(p);
        Token *n = expect(p, TOK_IDENT, "expected variable name after type");
        name = strtab_intern(p->strtab, n->text, n->text_len);
        if (match(p, TOK_ASSIGN)) {
            init_expr = parse_expression(p);
        }
        break;
    }
    }

    AstNode *node = ast_new(p->arena, NODE_VAR_DECL, loc);
    node->as.var_decl.decl_kind = kind;
    node->as.var_decl.name = name;
    node->as.var_decl.type_node = type_node;
    node->as.var_decl.init_expr = init_expr;
    return node;
}

/* ------------------------------------------------------------------ */
/*  asm TARGET { ... }                                                 */
/* ------------------------------------------------------------------ */
static AstNode *parse_asm_block(Parser *p)
{
    Token *asm_tok = advance(p);  /* consume 'asm' */
    Token *tgt_tok = expect(p, TOK_IDENT, "expected target (linux/windows/macos) after 'asm'");

    AsmTarget target = ASM_TARGET_LINUX;
    if (tgt_tok->text_len == 5 && memcmp(tgt_tok->text, "linux", 5) == 0) {
        target = ASM_TARGET_LINUX;
    } else if (tgt_tok->text_len == 7 && memcmp(tgt_tok->text, "windows", 7) == 0) {
        target = ASM_TARGET_WINDOWS;
    } else if (tgt_tok->text_len == 5 && memcmp(tgt_tok->text, "macos", 5) == 0) {
        target = ASM_TARGET_MACOS;
    } else {
        parser_error(p, tgt_tok, "expected 'linux', 'windows', or 'macos' as asm target");
    }

    expect(p, TOK_LBRACE, "expected '{' after asm target");

    /* The lexer emits a TOK_ASM_BODY token containing the raw text */
    const char *body = "";
    int body_len = 0;
    if (check(p, TOK_ASM_BODY)) {
        Token *body_tok = advance(p);
        body = body_tok->text;
        body_len = (int)body_tok->text_len;
    }

    expect(p, TOK_RBRACE, "expected '}' after asm block");

    AstNode *node = ast_new(p->arena, NODE_ASM_BLOCK, asm_tok->loc);
    node->as.asm_block.target = target;
    node->as.asm_block.body = body;
    node->as.asm_block.body_len = body_len;
    return node;
}

/* ------------------------------------------------------------------ */
/*  Expression parsing — Pratt-style precedence climbing               */
/* ------------------------------------------------------------------ */
static AstNode *parse_expression(Parser *p)
{
    return parse_assignment(p);
}

static AstNode *parse_assignment(Parser *p)
{
    if (check(p, TOK_IDENT) &&
        p->pos + 1 < p->token_count &&
        p->tokens[p->pos + 1].kind == TOK_ASSIGN) {
        Token *name_tok = advance(p);
        advance(p);  /* consume '=' */
        AstNode *value = parse_assignment(p);
        AstNode *node = ast_new(p->arena, NODE_ASSIGN_EXPR, name_tok->loc);
        node->as.assign.name = strtab_intern(p->strtab, name_tok->text, name_tok->text_len);
        node->as.assign.value = value;
        return node;
    }
    return parse_logic_or(p);
}

static AstNode *parse_logic_or(Parser *p)
{
    AstNode *left = parse_logic_and(p);
    while (match(p, TOK_OR)) {
        Token *op_tok = previous(p);
        AstNode *right = parse_logic_and(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = BINOP_LOGIC_OR;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_logic_and(Parser *p)
{
    AstNode *left = parse_bit_or(p);
    while (match(p, TOK_AND)) {
        Token *op_tok = previous(p);
        AstNode *right = parse_bit_or(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = BINOP_LOGIC_AND;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_bit_or(Parser *p)
{
    AstNode *left = parse_bit_xor(p);
    while (match(p, TOK_PIPE)) {
        Token *op_tok = previous(p);
        AstNode *right = parse_bit_xor(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = BINOP_BIT_OR;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_bit_xor(Parser *p)
{
    AstNode *left = parse_bit_and(p);
    while (match(p, TOK_CARET)) {
        Token *op_tok = previous(p);
        AstNode *right = parse_bit_and(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = BINOP_BIT_XOR;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_bit_and(Parser *p)
{
    AstNode *left = parse_equality(p);
    while (match(p, TOK_AMP)) {
        Token *op_tok = previous(p);
        AstNode *right = parse_equality(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = BINOP_BIT_AND;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_equality(Parser *p)
{
    AstNode *left = parse_comparison(p);
    while (check(p, TOK_EQUAL) || check(p, TOK_NOT_EQUAL)) {
        Token *op_tok = advance(p);
        BinOp op = (op_tok->kind == TOK_EQUAL) ? BINOP_EQ : BINOP_NE;
        AstNode *right = parse_comparison(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = op;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_comparison(Parser *p)
{
    AstNode *left = parse_shift(p);
    while (check(p, TOK_LESS) || check(p, TOK_GREATER) ||
           check(p, TOK_LESS_EQ) || check(p, TOK_GREATER_EQ)) {
        Token *op_tok = advance(p);
        BinOp op;
        switch (op_tok->kind) {
        case TOK_LESS:       op = BINOP_LT; break;
        case TOK_GREATER:    op = BINOP_GT; break;
        case TOK_LESS_EQ:    op = BINOP_LE; break;
        case TOK_GREATER_EQ: op = BINOP_GE; break;
        default:             op = BINOP_LT; break; /* unreachable */
        }
        AstNode *right = parse_shift(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = op;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_shift(Parser *p)
{
    AstNode *left = parse_additive(p);
    while (check(p, TOK_SHL) || check(p, TOK_SHR)) {
        Token *op_tok = advance(p);
        BinOp op = (op_tok->kind == TOK_SHL) ? BINOP_SHL : BINOP_SHR;
        AstNode *right = parse_additive(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = op;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_additive(Parser *p)
{
    AstNode *left = parse_multiplicative(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token *op_tok = advance(p);
        BinOp op = (op_tok->kind == TOK_PLUS) ? BINOP_ADD : BINOP_SUB;
        AstNode *right = parse_multiplicative(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = op;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_multiplicative(Parser *p)
{
    AstNode *left = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        Token *op_tok = advance(p);
        BinOp op;
        switch (op_tok->kind) {
        case TOK_STAR:    op = BINOP_MUL; break;
        case TOK_SLASH:   op = BINOP_DIV; break;
        case TOK_PERCENT: op = BINOP_MOD; break;
        default:          op = BINOP_MUL; break; /* unreachable */
        }
        AstNode *right = parse_unary(p);
        AstNode *node = ast_new(p->arena, NODE_BINARY_EXPR, op_tok->loc);
        node->as.binary.op = op;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }
    return left;
}

static AstNode *parse_unary(Parser *p)
{
    if (match(p, TOK_BANG)) {
        Token *op_tok = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new(p->arena, NODE_UNARY_EXPR, op_tok->loc);
        node->as.unary.op = UNOP_NOT;
        node->as.unary.operand = operand;
        return node;
    }
    if (match(p, TOK_TILDE)) {
        Token *op_tok = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new(p->arena, NODE_UNARY_EXPR, op_tok->loc);
        node->as.unary.op = UNOP_BITNOT;
        node->as.unary.operand = operand;
        return node;
    }
    if (match(p, TOK_MINUS)) {
        Token *op_tok = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new(p->arena, NODE_UNARY_EXPR, op_tok->loc);
        node->as.unary.op = UNOP_NEG;
        node->as.unary.operand = operand;
        return node;
    }
    return parse_call(p);
}

/* ------------------------------------------------------------------ */
/*  Call expressions: primary ( args )*                                */
/* ------------------------------------------------------------------ */
static AstNode *parse_call(Parser *p)
{
    AstNode *expr = parse_primary(p);

    while (match(p, TOK_LPAREN)) {
        Token *paren = previous(p);
        /* The callee must be an identifier for function calls */
        if (expr->kind != NODE_IDENT) {
            parser_error(p, paren, "expected function name before '('");
            /* consume until ) for recovery */
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                advance(p);
            }
            expect(p, TOK_RPAREN, "expected ')' after arguments");
            continue;
        }

        /* Collect arguments */
        int arg_cap = 8;
        int arg_count = 0;
        AstNode **args = (AstNode **)arena_alloc(p->arena, sizeof(AstNode *) * (size_t)arg_cap);

        if (!check(p, TOK_RPAREN)) {
            do {
                if (arg_count >= arg_cap) {
                    int new_cap = arg_cap * 2;
                    AstNode **new_args = (AstNode **)arena_alloc(p->arena, sizeof(AstNode *) * (size_t)new_cap);
                    memcpy(new_args, args, sizeof(AstNode *) * (size_t)arg_count);
                    args = new_args;
                    arg_cap = new_cap;
                }
                args[arg_count++] = parse_expression(p);
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RPAREN, "expected ')' after arguments");

        AstNode *call = ast_new(p->arena, NODE_CALL_EXPR, paren->loc);
        call->as.call.func_name = expr->as.ident.name;
        call->as.call.args = args;
        call->as.call.arg_count = arg_count;
        expr = call;
    }
    return expr;
}

/* ------------------------------------------------------------------ */
/*  Primary expressions                                                */
/* ------------------------------------------------------------------ */
static AstNode *parse_primary(Parser *p)
{
    /* Integer literal */
    if (match(p, TOK_INT_LIT)) {
        Token *tok = previous(p);
        AstNode *node = ast_new(p->arena, NODE_INT_LIT, tok->loc);
        node->as.int_lit.value = tok->int_value;
        return node;
    }

    /* Boolean literals */
    if (match(p, TOK_TRUE)) {
        Token *tok = previous(p);
        AstNode *node = ast_new(p->arena, NODE_BOOL_LIT, tok->loc);
        node->as.bool_lit.value = true;
        return node;
    }
    if (match(p, TOK_FALSE)) {
        Token *tok = previous(p);
        AstNode *node = ast_new(p->arena, NODE_BOOL_LIT, tok->loc);
        node->as.bool_lit.value = false;
        return node;
    }
    if (match(p, TOK_BOOL_LIT)) {
        Token *tok = previous(p);
        AstNode *node = ast_new(p->arena, NODE_BOOL_LIT, tok->loc);
        node->as.bool_lit.value = tok->bool_value;
        return node;
    }

    /* String literal */
    if (match(p, TOK_STRING_LIT)) {
        Token *tok = previous(p);
        AstNode *node = ast_new(p->arena, NODE_STRING_LIT, tok->loc);
        node->as.string_lit.text = tok->text;
        node->as.string_lit.len = tok->text_len;
        node->as.string_lit.data_id = -1; /* assigned by codegen */
        return node;
    }

    /* Identifier or type-cast */
    if (check(p, TOK_IDENT)) {
        const char *name = strtab_intern(p->strtab, current(p)->text, current(p)->text_len);
        /* Type cast: TypeName(expr) */
        if (is_type_name(p, name) &&
            p->pos + 1 < p->token_count &&
            p->tokens[p->pos + 1].kind == TOK_LPAREN) {
            AstNode *type_node = parse_type(p);
            return parse_cast(p, type_node);
        }
        /* Plain identifier */
        Token *tok = advance(p);
        AstNode *node = ast_new(p->arena, NODE_IDENT, tok->loc);
        node->as.ident.name = name;
        return node;
    }

    /* bit cast: bit(expr) or bit[n](expr) */
    if (check(p, TOK_BIT) &&
        p->pos + 1 < p->token_count &&
        (p->tokens[p->pos + 1].kind == TOK_LPAREN ||
         p->tokens[p->pos + 1].kind == TOK_LBRACKET)) {
        AstNode *type_node = parse_type(p);
        if (check(p, TOK_LPAREN)) {
            return parse_cast(p, type_node);
        }
        /* If not followed by '(', just return the type node (unusual) */
        return type_node;
    }

    /* Parenthesised expression */
    if (match(p, TOK_LPAREN)) {
        AstNode *expr = parse_expression(p);
        expect(p, TOK_RPAREN, "expected ')' after expression");
        return expr;
    }

    /* Error recovery */
    parser_error(p, current(p), "expected expression");
    Token *tok = current(p);
    advance(p);  /* skip the bad token to avoid infinite loops */
    return ast_new(p->arena, NODE_INT_LIT, tok->loc);
}

/* ------------------------------------------------------------------ */
/*  Cast: ( expr ) — the type has already been consumed                */
/* ------------------------------------------------------------------ */
static AstNode *parse_cast(Parser *p, AstNode *type_node)
{
    expect(p, TOK_LPAREN, "expected '(' for cast expression");
    AstNode *expr = parse_expression(p);
    Token *close = expect(p, TOK_RPAREN, "expected ')' after cast expression");
    (void)close;

    AstNode *node = ast_new(p->arena, NODE_CAST_EXPR, type_node->loc);
    node->as.cast.target_type = type_node;
    node->as.cast.expr = expr;
    return node;
}
