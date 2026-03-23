#ifndef HPP_AST_H
#define HPP_AST_H

#include "common/error.h"
#include "common/arena.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct AstNode AstNode;
typedef struct AstNodeList AstNodeList;
typedef struct HppType HppType;

typedef enum {
    // Top-level
    NODE_PROGRAM, NODE_TYPE_DEF, NODE_FUNC_DEF, NODE_OPP_DEF,
    NODE_IMPORT, NODE_LINK,
    // Statements
    NODE_BLOCK, NODE_VAR_DECL, NODE_EXPR_STMT, NODE_RETURN_STMT,
    NODE_IF_STMT, NODE_WHILE_STMT, NODE_FOR_STMT,
    NODE_BREAK_STMT, NODE_CONTINUE_STMT, NODE_ASM_BLOCK,
    // Expressions
    NODE_INT_LIT, NODE_BOOL_LIT, NODE_STRING_LIT, NODE_IDENT,
    NODE_BINARY_EXPR, NODE_UNARY_EXPR, NODE_CALL_EXPR,
    NODE_ASSIGN_EXPR, NODE_CAST_EXPR, NODE_ADDR_OF,
    // Types
    NODE_TYPE_BIT, NODE_TYPE_NAMED,
} AstNodeKind;

typedef enum { DECL_CONST, DECL_LET, DECL_TYPED } DeclKind;

typedef enum {
    BINOP_ADD, BINOP_SUB, BINOP_MUL, BINOP_DIV, BINOP_MOD,
    BINOP_BIT_AND, BINOP_BIT_OR, BINOP_BIT_XOR, BINOP_SHL, BINOP_SHR,
    BINOP_EQ, BINOP_NE, BINOP_LT, BINOP_GT, BINOP_LE, BINOP_GE,
    BINOP_LOGIC_AND, BINOP_LOGIC_OR,
} BinOp;

typedef enum { UNOP_NEG, UNOP_NOT, UNOP_BITNOT } UnOp;
typedef enum { ASM_TARGET_LINUX, ASM_TARGET_WINDOWS, ASM_TARGET_MACOS } AsmTarget;

struct AstNodeList {
    AstNode *node;
    AstNodeList *next;
};

typedef struct {
    const char *name;
    AstNode *type_node;
    SourceLoc loc;
} Param;

typedef struct {
    Param *params;
    int count;
} ParamList;

struct AstNode {
    AstNodeKind kind;
    SourceLoc loc;
    HppType *resolved_type;

    union {
        struct { AstNodeList *decls; } program;
        struct { const char *name; uint32_t bit_width; } type_def;
        /* import std; import std/io; import std/{io,mem}; */
        struct {
            const char **segments;  /* path segments: ["std","io"] */
            int segment_count;
            const char **names;     /* for multi-import: ["io","mem"] or NULL */
            int name_count;         /* 0 = import whole path */
        } import_decl;
        /* link "path.o"; */
        struct { const char *path; } link_decl;
        struct { const char *name; ParamList params; AstNode *return_type; AstNode *body; } func_def;
        struct {
            AstNode *left_type;
            BinOp op;
            bool is_unary;
            UnOp unary_op;
            AstNode *right_type;
            AstNode *result_type;
            AstNodeList *asm_blocks;
        } opp_def;
        struct { AstNodeList *stmts; } block;
        struct { DeclKind decl_kind; const char *name; AstNode *type_node; AstNode *init_expr; } var_decl;
        struct { AstNode *expr; } expr_stmt;
        struct { AstNode *expr; } return_stmt;
        struct { AstNode *condition; AstNode *then_block; AstNode *else_block; } if_stmt;
        struct { AstNode *condition; AstNode *body; } while_stmt;
        struct { AstNode *init; AstNode *condition; AstNode *update; AstNode *body; } for_stmt;
        struct { AsmTarget target; const char *body; int body_len; } asm_block;
        struct { uint64_t value; } int_lit;
        struct { bool value; } bool_lit;
        struct { const char *text; size_t len; int data_id; } string_lit;
        struct { const char *name; } ident;
        struct { BinOp op; AstNode *left; AstNode *right; } binary;
        struct { UnOp op; AstNode *operand; } unary;
        struct { const char *func_name; AstNode **args; int arg_count; } call;
        struct { const char *name; AstNode *value; } assign;
        struct { AstNode *target_type; AstNode *expr; } cast;
        struct { const char *name; } addr_of;  /* &variable */
        struct { uint32_t width; } type_bit;   // 0 means plain 'bit' = bit[1]
        struct { const char *name; } type_named;
    } as;
};

AstNode     *ast_new(Arena *arena, AstNodeKind kind, SourceLoc loc);
AstNodeList *ast_list_append(Arena *arena, AstNodeList *list, AstNode *node);
int          ast_list_len(AstNodeList *list);
void         ast_print(AstNode *node, int indent);
const char  *ast_node_kind_str(AstNodeKind kind);
const char  *binop_str(BinOp op);
const char  *unop_str(UnOp op);

#endif
