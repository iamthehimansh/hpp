#ifndef HPP_MACRO_H
#define HPP_MACRO_H

#include "common/arena.h"
#include "lexer/token.h"
#include <stdbool.h>

#define MAX_MACROS 256
#define MAX_MACRO_PARAMS 16
#define MAX_MACRO_BODY 1024
#define MAX_ARRAY_VARS 256

typedef struct {
    const char *name;                       /* interned macro name */
    const char *params[MAX_MACRO_PARAMS];   /* parameter names (interned) */
    int param_count;
    Token body[MAX_MACRO_BODY];             /* body tokens */
    int body_count;
} MacroDef;

/* Tracks which variables are arrays and their element type */
typedef struct {
    const char *var_name;   /* variable name */
    const char *type_name;  /* element type: "int", "byte", "long" */
} ArrayVarEntry;

typedef struct {
    Arena *arena;
    MacroDef macros[MAX_MACROS];
    int macro_count;
    ArrayVarEntry array_vars[MAX_ARRAY_VARS];
    int array_var_count;
} MacroProcessor;

/* Create the macro processor */
MacroProcessor *macro_create(Arena *arena);

/* Process a token array: extract macro definitions, expand macro calls.
   Returns a new token array (arena-allocated) with macros expanded.
   Updates *count with the new token count. */
Token *macro_process(MacroProcessor *mp, Token *tokens, int count, int *out_count);

#endif
