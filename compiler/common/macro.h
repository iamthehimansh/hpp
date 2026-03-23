#ifndef HPP_MACRO_H
#define HPP_MACRO_H

#include "common/arena.h"
#include "lexer/token.h"
#include <stdbool.h>

#define MAX_MACROS 256
#define MAX_MACRO_PARAMS 16
#define MAX_MACRO_BODY 1024
#define MAX_ARRAY_VARS 256
#define MAX_TYPE_DEFS 128
#define MAX_STRUCT_DEFS 64
#define MAX_STRUCT_FIELDS 32
#define MAX_ENUM_DEFS 64
#define MAX_ENUM_MEMBERS 512
#define MAX_CONSTANTS 256

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

/* Tracks user-defined type widths from def statements */
typedef struct {
    const char *type_name;  /* e.g., "short", "word" */
    int bit_width;          /* e.g., 16 */
} TypeWidthEntry;

/* A field in a defx struct */
typedef struct {
    const char *name;       /* field name */
    const char *type_name;  /* field type: "int", "byte", "long", etc. */
    int bit_width;          /* field size in bits */
    int byte_offset;        /* byte offset from start of struct */
} StructField;

/* A defx struct definition */
typedef struct {
    const char *name;                       /* struct name */
    StructField fields[MAX_STRUCT_FIELDS];  /* fields */
    int field_count;
    int total_size;                         /* total size in bytes */
} StructDef;

/* Tracks which variables are instances of which struct */
typedef struct {
    const char *var_name;
    const char *struct_name;
} StructVarEntry;

/* Enum member */
typedef struct { const char *name; int value; } EnumMember;

/* Enum definition */
typedef struct {
    const char *name;
    EnumMember members[MAX_ENUM_MEMBERS];
    int member_count;
} EnumDef;

/* Top-level constant */
typedef struct { const char *name; uint64_t value; } ConstantDef;

typedef struct {
    Arena *arena;
    MacroDef macros[MAX_MACROS];
    int macro_count;
    ArrayVarEntry array_vars[MAX_ARRAY_VARS];
    int array_var_count;
    TypeWidthEntry type_widths[MAX_TYPE_DEFS];
    int type_width_count;
    StructDef structs[MAX_STRUCT_DEFS];
    int struct_count;
    StructVarEntry struct_vars[MAX_ARRAY_VARS];
    int struct_var_count;
    EnumDef enums[MAX_ENUM_DEFS];
    int enum_count;
    ConstantDef constants[MAX_CONSTANTS];
    int constant_count;
} MacroProcessor;

/* Create the macro processor */
MacroProcessor *macro_create(Arena *arena);

/* Process a token array: extract macro definitions, expand macro calls.
   Returns a new token array (arena-allocated) with macros expanded.
   Updates *count with the new token count. */
Token *macro_process(MacroProcessor *mp, Token *tokens, int count, int *out_count);

#endif
