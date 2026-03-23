#include "common/macro.h"
#include "common/error.h"
#include <string.h>
#include <stdio.h>

MacroProcessor *macro_create(Arena *arena) {
    MacroProcessor *mp = arena_alloc(arena, sizeof(MacroProcessor));
    mp->arena = arena;
    mp->macro_count = 0;
    return mp;
}

/* Find a macro by name (pointer comparison, interned) */
static MacroDef *find_macro(MacroProcessor *mp, const char *name) {
    for (int i = 0; i < mp->macro_count; i++) {
        if (strcmp(mp->macros[i].name, name) == 0) {
            return &mp->macros[i];
        }
    }
    return NULL;
}

/* ---- Pass 1: Extract macro definitions from token stream ---- */

static Token *extract_macros(MacroProcessor *mp, Token *tokens, int count,
                              int *out_count) {
    /* Output: tokens with macro definitions removed */
    Token *out = arena_alloc(mp->arena, (size_t)count * sizeof(Token));
    int out_pos = 0;

    for (int i = 0; i < count; ) {
        if (tokens[i].kind == TOK_MACRO) {
            /* Parse: macro NAME ( params ) { body } */
            i++; /* skip 'macro' */

            if (i >= count || tokens[i].kind != TOK_IDENT) {
                error_report(tokens[i > 0 ? i-1 : 0].loc, ERR_PARSER,
                             "expected macro name after 'macro'");
                i++;
                continue;
            }

            if (mp->macro_count >= MAX_MACROS) {
                error_report(tokens[i].loc, ERR_PARSER, "too many macros");
                i++;
                continue;
            }

            MacroDef *m = &mp->macros[mp->macro_count];
            m->name = tokens[i].text;
            m->param_count = 0;
            m->body_count = 0;
            i++; /* skip name */

            /* Parse parameters */
            if (i < count && tokens[i].kind == TOK_LPAREN) {
                i++; /* skip '(' */
                while (i < count && tokens[i].kind != TOK_RPAREN) {
                    if (tokens[i].kind == TOK_IDENT) {
                        if (m->param_count < MAX_MACRO_PARAMS) {
                            m->params[m->param_count++] = tokens[i].text;
                        }
                    }
                    i++;
                    if (i < count && tokens[i].kind == TOK_COMMA) i++;
                }
                if (i < count) i++; /* skip ')' */
            }

            /* Parse body: { tokens } with brace nesting */
            if (i < count && tokens[i].kind == TOK_LBRACE) {
                i++; /* skip '{' */
                int depth = 1;
                while (i < count && depth > 0) {
                    if (tokens[i].kind == TOK_LBRACE) depth++;
                    else if (tokens[i].kind == TOK_RBRACE) {
                        depth--;
                        if (depth == 0) break;
                    }
                    if (m->body_count < MAX_MACRO_BODY) {
                        m->body[m->body_count++] = tokens[i];
                    }
                    i++;
                }
                if (i < count) i++; /* skip closing '}' */
            }

            mp->macro_count++;
        } else {
            out[out_pos++] = tokens[i++];
        }
    }

    *out_count = out_pos;
    return out;
}

/* ---- Pass 2: Expand macro invocations ---- */

/* Check if token at pos is a macro call: NAME ( args ) */
static bool is_macro_call(MacroProcessor *mp, Token *tokens, int count,
                           int pos) {
    if (pos >= count) return false;
    if (tokens[pos].kind != TOK_IDENT) return false;
    if (find_macro(mp, tokens[pos].text) == NULL) return false;
    if (pos + 1 >= count) return false;
    /* Macro call requires '(' after name */
    if (tokens[pos + 1].kind != TOK_LPAREN) return false;
    return true;
}

/* Parse arguments of a macro call: NAME( arg1, arg2, ... )
   Each argument is a sequence of tokens, separated by commas.
   Handles nested parentheses.
   Returns the position after the closing ')'. */
static int parse_macro_args(Token *tokens, int count, int start,
                             Token args[][256], int arg_counts[], int *nargs) {
    int pos = start; /* should point to '(' */
    if (pos >= count || tokens[pos].kind != TOK_LPAREN) {
        *nargs = 0;
        return pos;
    }
    pos++; /* skip '(' */

    *nargs = 0;
    if (pos < count && tokens[pos].kind == TOK_RPAREN) {
        pos++; /* empty args */
        return pos;
    }

    int cur_arg = 0;
    arg_counts[cur_arg] = 0;
    int depth = 0;

    while (pos < count) {
        if (tokens[pos].kind == TOK_LPAREN) depth++;
        else if (tokens[pos].kind == TOK_RPAREN) {
            if (depth == 0) {
                cur_arg++;
                pos++; /* skip ')' */
                break;
            }
            depth--;
        } else if (tokens[pos].kind == TOK_COMMA && depth == 0) {
            cur_arg++;
            if (cur_arg < MAX_MACRO_PARAMS) arg_counts[cur_arg] = 0;
            pos++;
            continue;
        }

        if (cur_arg < MAX_MACRO_PARAMS && arg_counts[cur_arg] < 256) {
            args[cur_arg][arg_counts[cur_arg]++] = tokens[pos];
        }
        pos++;
    }

    *nargs = cur_arg;
    return pos;
}

/* Expand macros in the token stream. Returns new token array. */
static Token *expand_macros(MacroProcessor *mp, Token *tokens, int count,
                             int *out_count) {
    /* Allocate generous output buffer */
    int cap = count * 4 + 1024;
    Token *out = arena_alloc(mp->arena, (size_t)cap * sizeof(Token));
    int out_pos = 0;

    for (int i = 0; i < count; ) {
        if (is_macro_call(mp, tokens, count, i)) {
            MacroDef *m = find_macro(mp, tokens[i].text);
            SourceLoc call_loc = tokens[i].loc;
            i++; /* skip macro name */

            /* Parse arguments */
            Token args[MAX_MACRO_PARAMS][256];
            int arg_counts[MAX_MACRO_PARAMS];
            int nargs = 0;
            i = parse_macro_args(tokens, count, i, args, arg_counts, &nargs);

            if (nargs != m->param_count) {
                error_report(call_loc, ERR_PARSER,
                             "macro '%s' expects %d args, got %d",
                             m->name, m->param_count, nargs);
                continue;
            }

            /* Substitute body tokens, replacing param names with arg tokens */
            for (int b = 0; b < m->body_count && out_pos < cap - 256; b++) {
                bool substituted = false;
                if (m->body[b].kind == TOK_IDENT) {
                    for (int p = 0; p < m->param_count; p++) {
                        if (strcmp(m->body[b].text, m->params[p]) == 0) {
                            /* Replace with argument tokens */
                            for (int a = 0; a < arg_counts[p]; a++) {
                                out[out_pos] = args[p][a];
                                out[out_pos].loc = call_loc;
                                out_pos++;
                            }
                            substituted = true;
                            break;
                        }
                    }
                }
                if (!substituted) {
                    out[out_pos] = m->body[b];
                    out[out_pos].loc = call_loc;
                    out_pos++;
                }
            }
        } else {
            out[out_pos++] = tokens[i++];
        }
    }

    *out_count = out_pos;
    return out;
}

/* ---- Pass: Rewrite array declarations ---- */
/* int arr[5];           → long arr = int_new(5);                          */
/* int arr[] = [1,2,3];  → long arr = int_new(3); arr[0]=1; arr[1]=2; ... */
/* int arr[3] = [1,2,3]; → long arr = int_new(3); arr[0]=1; arr[1]=2; ... */

static Token make_synth_token(SourceLoc loc, TokenKind kind, const char *text, size_t len) {
    Token t;
    memset(&t, 0, sizeof(t));
    t.kind = kind;
    t.loc = loc;
    t.text = text;
    t.text_len = len;
    return t;
}

static Token make_int_token(SourceLoc loc, uint64_t val, Arena *arena) {
    Token t;
    memset(&t, 0, sizeof(t));
    t.kind = TOK_INT_LIT;
    t.loc = loc;
    t.int_value = val;
    /* Create text representation */
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%d", (int)val);
    t.text = arena_strndup(arena, buf, (size_t)n);
    t.text_len = (size_t)n;
    return t;
}

/* Check if position has pattern: TYPE_IDENT VAR_IDENT [ */
static bool is_array_decl(Token *tokens, int count, int pos) {
    if (pos + 2 >= count) return false;
    if (tokens[pos].kind != TOK_IDENT) return false;
    if (tokens[pos + 1].kind != TOK_IDENT) return false;
    if (tokens[pos + 2].kind != TOK_LBRACKET) return false;
    /* Make sure first ident isn't a keyword-like context */
    /* Check we're not inside an expression (prev token isn't operator/assign) */
    if (pos > 0) {
        TokenKind prev = tokens[pos - 1].kind;
        if (prev == TOK_ASSIGN || prev == TOK_PLUS || prev == TOK_MINUS ||
            prev == TOK_STAR || prev == TOK_SLASH || prev == TOK_COMMA ||
            prev == TOK_LPAREN || prev == TOK_EQUAL || prev == TOK_NOT_EQUAL ||
            prev == TOK_LESS || prev == TOK_GREATER || prev == TOK_RETURN)
            return false;
    }
    return true;
}

/* Build a function name like "int_new" from type name "int" and suffix "_new" */
static const char *build_func_name(Arena *arena, const char *type, const char *suffix) {
    size_t tlen = strlen(type);
    size_t slen = strlen(suffix);
    char *buf = arena_alloc(arena, tlen + slen + 1);
    memcpy(buf, type, tlen);
    memcpy(buf + tlen, suffix, slen);
    buf[tlen + slen] = '\0';
    return buf;
}

static Token *rewrite_array_decls(MacroProcessor *mp, Token *tokens, int count,
                                   int *out_count) {
    int cap = count * 6 + 1024;
    Token *out = arena_alloc(mp->arena, (size_t)cap * sizeof(Token));
    int op = 0;  /* output position */

    for (int i = 0; i < count; ) {
        if (!is_array_decl(tokens, count, i)) {
            out[op++] = tokens[i++];
            continue;
        }

        SourceLoc loc = tokens[i].loc;
        const char *type_name = tokens[i].text;
        const char *var_name = tokens[i + 1].text;
        size_t var_len = tokens[i + 1].text_len;
        i += 2; /* skip TYPE and NAME */

        const char *new_fn = build_func_name(mp->arena, type_name, "_new");
        size_t new_fn_len = strlen(new_fn);

        /* Now at '[' */
        i++; /* skip '[' */

        /* Collect size expression (may be empty for []) */
        Token size_tokens[64];
        int size_len = 0;
        while (i < count && tokens[i].kind != TOK_RBRACKET && size_len < 64) {
            size_tokens[size_len++] = tokens[i++];
        }
        if (i < count) i++; /* skip ']' */

        /* Check for initializer: = [ val1, val2, ... ] */
        Token init_vals[256];
        int init_count = 0;
        bool has_init = false;

        if (i < count && tokens[i].kind == TOK_ASSIGN) {
            i++; /* skip '=' */
            if (i < count && tokens[i].kind == TOK_LBRACKET) {
                i++; /* skip '[' */
                has_init = true;
                int depth = 0;
                /* Collect comma-separated values */
                while (i < count && !(tokens[i].kind == TOK_RBRACKET && depth == 0)) {
                    if (tokens[i].kind == TOK_LBRACKET) depth++;
                    if (tokens[i].kind == TOK_RBRACKET) depth--;
                    if (tokens[i].kind == TOK_COMMA && depth == 0) {
                        i++; /* skip comma between values */
                        continue;
                    }
                    if (init_count < 256) {
                        init_vals[init_count++] = tokens[i];
                    }
                    i++;
                }
                if (i < count) i++; /* skip ']' */
            }
        }
        if (i < count && tokens[i].kind == TOK_SEMICOLON) i++; /* skip ';' */

        /* Determine size */
        int known_size = init_count; /* from initializer count */
        bool use_init_size = (size_len == 0 && has_init);

        /* Emit: long VAR = TYPE_new(SIZE); */
        out[op++] = make_synth_token(loc, TOK_IDENT, "long", 4);
        out[op++] = make_synth_token(loc, TOK_IDENT, var_name, var_len);
        out[op++] = make_synth_token(loc, TOK_ASSIGN, "=", 1);
        out[op++] = make_synth_token(loc, TOK_IDENT, new_fn, new_fn_len);
        out[op++] = make_synth_token(loc, TOK_LPAREN, "(", 1);
        if (use_init_size) {
            out[op++] = make_int_token(loc, (uint64_t)known_size, mp->arena);
        } else {
            for (int j = 0; j < size_len; j++)
                out[op++] = size_tokens[j];
        }
        out[op++] = make_synth_token(loc, TOK_RPAREN, ")", 1);
        out[op++] = make_synth_token(loc, TOK_SEMICOLON, ";", 1);

        /* Emit initializer: TYPE_set(VAR, 0, val0); TYPE_set(VAR, 1, val1); ... */
        if (has_init) {
            const char *set_fn = build_func_name(mp->arena, type_name, "_set");
            size_t set_fn_len = strlen(set_fn);
            for (int j = 0; j < init_count && op < cap - 16; j++) {
                out[op++] = make_synth_token(loc, TOK_IDENT, set_fn, set_fn_len);
                out[op++] = make_synth_token(loc, TOK_LPAREN, "(", 1);
                out[op++] = make_synth_token(loc, TOK_IDENT, var_name, var_len);
                out[op++] = make_synth_token(loc, TOK_COMMA, ",", 1);
                out[op++] = make_int_token(loc, (uint64_t)j, mp->arena);
                out[op++] = make_synth_token(loc, TOK_COMMA, ",", 1);
                out[op++] = init_vals[j];
                out[op++] = make_synth_token(loc, TOK_RPAREN, ")", 1);
                out[op++] = make_synth_token(loc, TOK_SEMICOLON, ";", 1);
            }
        }
    }

    *out_count = op;
    return out;
}

/* ---- Pass: Rewrite bracket syntax ---- */
/* arr[i]       → int_get(arr, i)          */
/* arr[i] = val → int_set(arr, i, val)     */

/* Collect tokens between [ and matching ], handling nesting.
   Returns position after the ]. */
static int collect_bracket_expr(Token *tokens, int count, int start,
                                 Token *out_tokens, int *out_len) {
    int pos = start; /* should point to '[' */
    if (pos >= count || tokens[pos].kind != TOK_LBRACKET) {
        *out_len = 0;
        return pos;
    }
    pos++; /* skip '[' */
    int depth = 1;
    *out_len = 0;
    while (pos < count && depth > 0) {
        if (tokens[pos].kind == TOK_LBRACKET) depth++;
        else if (tokens[pos].kind == TOK_RBRACKET) {
            depth--;
            if (depth == 0) { pos++; break; }
        }
        if (*out_len < 256) {
            out_tokens[(*out_len)++] = tokens[pos];
        }
        pos++;
    }
    return pos;
}

/* Check if this IDENT[ pattern is an array access (not a type like bit[32]) */
static bool is_array_access(Token *tokens, int count, int pos) {
    if (pos >= count) return false;
    if (tokens[pos].kind != TOK_IDENT) return false;
    if (pos + 1 >= count) return false;
    if (tokens[pos + 1].kind != TOK_LBRACKET) return false;
    /* Check context: skip if preceded by 'def' (type definition) */
    if (pos > 0 && tokens[pos - 1].kind == TOK_DEF) return false;
    /* Skip if preceded by ':' (parameter type annotation) */
    if (pos > 0 && tokens[pos - 1].kind == TOK_COLON) return false;
    /* Skip if preceded by '->' (return type) */
    if (pos > 0 && tokens[pos - 1].kind == TOK_ARROW) return false;
    return true;
}

static Token *rewrite_brackets(MacroProcessor *mp, Token *tokens, int count,
                                int *out_count) {
    int cap = count * 4 + 256;
    Token *out = arena_alloc(mp->arena, (size_t)cap * sizeof(Token));
    int out_pos = 0;

    /* Intern the function names we'll need */
    const char *int_get_str = "int_get";
    const char *int_set_str = "int_set";

    for (int i = 0; i < count; ) {
        if (is_array_access(tokens, count, i)) {
            SourceLoc loc = tokens[i].loc;
            Token name_tok = tokens[i];
            i++; /* skip IDENT */

            /* Collect index expression between [ ] */
            Token idx_tokens[256];
            int idx_len = 0;
            i = collect_bracket_expr(tokens, count, i, idx_tokens, &idx_len);

            /* Check if followed by '=' (assignment) */
            if (i < count && tokens[i].kind == TOK_ASSIGN) {
                i++; /* skip '=' */

                /* Collect value expression until ; or ) or , or } */
                Token val_tokens[256];
                int val_len = 0;
                int depth = 0;
                while (i < count && val_len < 256) {
                    TokenKind k = tokens[i].kind;
                    if (k == TOK_LPAREN) depth++;
                    else if (k == TOK_RPAREN) {
                        if (depth == 0) break;
                        depth--;
                    }
                    else if (depth == 0 &&
                             (k == TOK_SEMICOLON || k == TOK_COMMA ||
                              k == TOK_RBRACE)) {
                        break;
                    }
                    val_tokens[val_len++] = tokens[i++];
                }

                /* Emit: int_set ( name , idx , val ) */
                out[out_pos++] = make_synth_token(loc, TOK_IDENT, int_set_str, 7);
                out[out_pos++] = make_synth_token(loc, TOK_LPAREN, "(", 1);
                out[out_pos++] = name_tok;
                out[out_pos++] = make_synth_token(loc, TOK_COMMA, ",", 1);
                for (int j = 0; j < idx_len && out_pos < cap - 16; j++)
                    out[out_pos++] = idx_tokens[j];
                out[out_pos++] = make_synth_token(loc, TOK_COMMA, ",", 1);
                for (int j = 0; j < val_len && out_pos < cap - 8; j++)
                    out[out_pos++] = val_tokens[j];
                out[out_pos++] = make_synth_token(loc, TOK_RPAREN, ")", 1);
            } else {
                /* Emit: int_get ( name , idx ) */
                out[out_pos++] = make_synth_token(loc, TOK_IDENT, int_get_str, 7);
                out[out_pos++] = make_synth_token(loc, TOK_LPAREN, "(", 1);
                out[out_pos++] = name_tok;
                out[out_pos++] = make_synth_token(loc, TOK_COMMA, ",", 1);
                for (int j = 0; j < idx_len && out_pos < cap - 8; j++)
                    out[out_pos++] = idx_tokens[j];
                out[out_pos++] = make_synth_token(loc, TOK_RPAREN, ")", 1);
            }
        } else {
            out[out_pos++] = tokens[i++];
        }
    }

    *out_count = out_pos;
    return out;
}

/* ---- Public API ---- */

Token *macro_process(MacroProcessor *mp, Token *tokens, int count, int *out_count) {
    /* Pass 1: extract macro definitions */
    int stripped_count = 0;
    Token *stripped = extract_macros(mp, tokens, count, &stripped_count);

    /* Pass 2: rewrite array declarations: int arr[5]; → long arr = int_new(5); */
    int decl_count = 0;
    Token *after_decls = rewrite_array_decls(mp, stripped, stripped_count, &decl_count);

    /* Pass 3: rewrite bracket syntax: arr[i] → int_get(arr, i)
       Run multiple passes for nested brackets like a[i] = a[j] * 2 */
    Token *rewritten = after_decls;
    int rewritten_count = decl_count;
    for (int pass = 0; pass < 8; pass++) {
        int new_count = 0;
        Token *rw = rewrite_brackets(mp, rewritten, rewritten_count, &new_count);
        if (new_count == rewritten_count) { rewritten = rw; rewritten_count = new_count; break; }
        rewritten = rw;
        rewritten_count = new_count;
    }

    /* Pass 3: expand macros (run up to 8 times for nested macros) */
    Token *current = rewritten;
    int current_count = rewritten_count;
    for (int pass = 0; pass < 8; pass++) {
        int new_count = 0;
        Token *expanded = expand_macros(mp, current, current_count, &new_count);
        if (new_count == current_count) {
            /* No more expansions */
            current = expanded;
            current_count = new_count;
            break;
        }
        current = expanded;
        current_count = new_count;
    }

    *out_count = current_count;
    return current;
}
