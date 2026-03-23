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

/* ---- Public API ---- */

Token *macro_process(MacroProcessor *mp, Token *tokens, int count, int *out_count) {
    /* Pass 1: extract macro definitions */
    int stripped_count = 0;
    Token *stripped = extract_macros(mp, tokens, count, &stripped_count);

    if (mp->macro_count == 0) {
        /* No macros — return stripped tokens as-is */
        *out_count = stripped_count;
        return stripped;
    }

    /* Pass 2: expand macros (run up to 8 times for nested macros) */
    Token *current = stripped;
    int current_count = stripped_count;
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
