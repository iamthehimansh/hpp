#include "ast/ast.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

AstNode *ast_new(Arena *arena, AstNodeKind kind, SourceLoc loc)
{
    AstNode *node = arena_alloc(arena, sizeof(AstNode));
    memset(node, 0, sizeof(AstNode));
    node->kind = kind;
    node->loc = loc;
    return node;
}

AstNodeList *ast_list_append(Arena *arena, AstNodeList *list, AstNode *node)
{
    AstNodeList *entry = arena_alloc(arena, sizeof(AstNodeList));
    entry->node = node;
    entry->next = NULL;

    if (!list) {
        return entry;
    }

    AstNodeList *cur = list;
    while (cur->next) {
        cur = cur->next;
    }
    cur->next = entry;
    return list;
}

int ast_list_len(AstNodeList *list)
{
    int count = 0;
    while (list) {
        count++;
        list = list->next;
    }
    return count;
}

const char *ast_node_kind_str(AstNodeKind kind)
{
    switch (kind) {
    case NODE_PROGRAM:       return "Program";
    case NODE_TYPE_DEF:      return "TypeDef";
    case NODE_FUNC_DEF:      return "FuncDef";
    case NODE_OPP_DEF:       return "OppDef";
    case NODE_IMPORT:        return "Import";
    case NODE_LINK:          return "Link";
    case NODE_BLOCK:         return "Block";
    case NODE_VAR_DECL:      return "VarDecl";
    case NODE_EXPR_STMT:     return "ExprStmt";
    case NODE_RETURN_STMT:   return "ReturnStmt";
    case NODE_IF_STMT:       return "IfStmt";
    case NODE_WHILE_STMT:    return "WhileStmt";
    case NODE_FOR_STMT:      return "ForStmt";
    case NODE_BREAK_STMT:    return "BreakStmt";
    case NODE_CONTINUE_STMT: return "ContinueStmt";
    case NODE_ASM_BLOCK:     return "AsmBlock";
    case NODE_INT_LIT:       return "IntLit";
    case NODE_BOOL_LIT:      return "BoolLit";
    case NODE_STRING_LIT:    return "StringLit";
    case NODE_IDENT:         return "Ident";
    case NODE_BINARY_EXPR:   return "BinaryExpr";
    case NODE_UNARY_EXPR:    return "UnaryExpr";
    case NODE_CALL_EXPR:     return "CallExpr";
    case NODE_ASSIGN_EXPR:   return "AssignExpr";
    case NODE_CAST_EXPR:     return "CastExpr";
    case NODE_ADDR_OF:       return "AddrOf";
    case NODE_TYPE_BIT:      return "TypeBit";
    case NODE_TYPE_NAMED:    return "TypeNamed";
    }
    return "Unknown";
}

const char *binop_str(BinOp op)
{
    switch (op) {
    case BINOP_ADD:       return "+";
    case BINOP_SUB:       return "-";
    case BINOP_MUL:       return "*";
    case BINOP_DIV:       return "/";
    case BINOP_MOD:       return "%";
    case BINOP_BIT_AND:   return "&";
    case BINOP_BIT_OR:    return "|";
    case BINOP_BIT_XOR:   return "^";
    case BINOP_SHL:       return "<<";
    case BINOP_SHR:       return ">>";
    case BINOP_EQ:        return "==";
    case BINOP_NE:        return "!=";
    case BINOP_LT:        return "<";
    case BINOP_GT:        return ">";
    case BINOP_LE:        return "<=";
    case BINOP_GE:        return ">=";
    case BINOP_LOGIC_AND: return "&&";
    case BINOP_LOGIC_OR:  return "||";
    }
    return "?";
}

const char *unop_str(UnOp op)
{
    switch (op) {
    case UNOP_NEG:    return "-";
    case UNOP_NOT:    return "!";
    case UNOP_BITNOT: return "~";
    }
    return "?";
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++) {
        putchar(' ');
    }
}

static const char *asm_target_str(AsmTarget target)
{
    switch (target) {
    case ASM_TARGET_LINUX:   return "linux";
    case ASM_TARGET_WINDOWS: return "windows";
    case ASM_TARGET_MACOS:   return "macos";
    }
    return "unknown";
}

static const char *decl_kind_str(DeclKind dk)
{
    switch (dk) {
    case DECL_CONST: return "const";
    case DECL_LET:   return "let";
    case DECL_TYPED: return "typed";
    }
    return "?";
}

void ast_print(AstNode *node, int indent)
{
    if (!node) {
        return;
    }

    print_indent(indent);

    switch (node->kind) {
    case NODE_PROGRAM:
        printf("Program\n");
        for (AstNodeList *n = node->as.program.decls; n; n = n->next) {
            ast_print(n->node, indent + 2);
        }
        break;

    case NODE_TYPE_DEF:
        printf("TypeDef '%s' = bit[%u]\n", node->as.type_def.name,
               node->as.type_def.bit_width);
        break;

    case NODE_FUNC_DEF:
        printf("FuncDef '%s'", node->as.func_def.name);
        if (node->as.func_def.return_type) {
            printf(" -> ");
            /* Print return type inline without newline */
        }
        printf("\n");
        if (node->as.func_def.return_type) {
            ast_print(node->as.func_def.return_type, indent + 2);
        }
        for (int i = 0; i < node->as.func_def.params.count; i++) {
            Param *p = &node->as.func_def.params.params[i];
            print_indent(indent + 2);
            printf("Param '%s'\n", p->name);
            ast_print(p->type_node, indent + 4);
        }
        ast_print(node->as.func_def.body, indent + 2);
        break;

    case NODE_OPP_DEF:
        if (node->as.opp_def.is_unary) {
            printf("OppDef unary '%s'\n", unop_str(node->as.opp_def.unary_op));
        } else {
            printf("OppDef binary '%s'\n", binop_str(node->as.opp_def.op));
        }
        ast_print(node->as.opp_def.left_type, indent + 2);
        if (node->as.opp_def.right_type) {
            ast_print(node->as.opp_def.right_type, indent + 2);
        }
        ast_print(node->as.opp_def.result_type, indent + 2);
        for (AstNodeList *n = node->as.opp_def.asm_blocks; n; n = n->next) {
            ast_print(n->node, indent + 2);
        }
        break;

    case NODE_IMPORT: {
        printf("Import ");
        for (int i = 0; i < node->as.import_decl.segment_count; i++) {
            if (i > 0) printf("/");
            printf("%s", node->as.import_decl.segments[i]);
        }
        if (node->as.import_decl.name_count > 0) {
            printf("/{");
            for (int i = 0; i < node->as.import_decl.name_count; i++) {
                if (i > 0) printf(", ");
                printf("%s", node->as.import_decl.names[i]);
            }
            printf("}");
        }
        printf("\n");
        break;
    }

    case NODE_LINK:
        printf("Link '%s'\n", node->as.link_decl.path);
        break;

    case NODE_BLOCK:
        printf("Block\n");
        for (AstNodeList *n = node->as.block.stmts; n; n = n->next) {
            ast_print(n->node, indent + 2);
        }
        break;

    case NODE_VAR_DECL:
        printf("VarDecl (%s) '%s'\n", decl_kind_str(node->as.var_decl.decl_kind),
               node->as.var_decl.name);
        ast_print(node->as.var_decl.type_node, indent + 2);
        ast_print(node->as.var_decl.init_expr, indent + 2);
        break;

    case NODE_EXPR_STMT:
        printf("ExprStmt\n");
        ast_print(node->as.expr_stmt.expr, indent + 2);
        break;

    case NODE_RETURN_STMT:
        printf("ReturnStmt\n");
        ast_print(node->as.return_stmt.expr, indent + 2);
        break;

    case NODE_IF_STMT:
        printf("IfStmt\n");
        ast_print(node->as.if_stmt.condition, indent + 2);
        ast_print(node->as.if_stmt.then_block, indent + 2);
        ast_print(node->as.if_stmt.else_block, indent + 2);
        break;

    case NODE_WHILE_STMT:
        printf("WhileStmt\n");
        ast_print(node->as.while_stmt.condition, indent + 2);
        ast_print(node->as.while_stmt.body, indent + 2);
        break;

    case NODE_FOR_STMT:
        printf("ForStmt\n");
        ast_print(node->as.for_stmt.init, indent + 2);
        ast_print(node->as.for_stmt.condition, indent + 2);
        ast_print(node->as.for_stmt.update, indent + 2);
        ast_print(node->as.for_stmt.body, indent + 2);
        break;

    case NODE_BREAK_STMT:
        printf("BreakStmt\n");
        break;

    case NODE_CONTINUE_STMT:
        printf("ContinueStmt\n");
        break;

    case NODE_ASM_BLOCK:
        printf("AsmBlock <%s>\n", asm_target_str(node->as.asm_block.target));
        break;

    case NODE_INT_LIT:
        printf("IntLit %" PRIu64 "\n", node->as.int_lit.value);
        break;

    case NODE_BOOL_LIT:
        printf("BoolLit %s\n", node->as.bool_lit.value ? "true" : "false");
        break;

    case NODE_STRING_LIT:
        printf("StringLit \"%.*s\"\n", (int)node->as.string_lit.len, node->as.string_lit.text);
        break;

    case NODE_IDENT:
        printf("Ident '%s'\n", node->as.ident.name);
        break;

    case NODE_BINARY_EXPR:
        printf("BinaryExpr <%s>\n", binop_str(node->as.binary.op));
        ast_print(node->as.binary.left, indent + 2);
        ast_print(node->as.binary.right, indent + 2);
        break;

    case NODE_UNARY_EXPR:
        printf("UnaryExpr <%s>\n", unop_str(node->as.unary.op));
        ast_print(node->as.unary.operand, indent + 2);
        break;

    case NODE_CALL_EXPR:
        printf("CallExpr '%s'\n", node->as.call.func_name);
        for (int i = 0; i < node->as.call.arg_count; i++) {
            ast_print(node->as.call.args[i], indent + 2);
        }
        break;

    case NODE_ASSIGN_EXPR:
        printf("AssignExpr '%s'\n", node->as.assign.name);
        ast_print(node->as.assign.value, indent + 2);
        break;

    case NODE_CAST_EXPR:
        printf("CastExpr\n");
        ast_print(node->as.cast.target_type, indent + 2);
        ast_print(node->as.cast.expr, indent + 2);
        break;

    case NODE_ADDR_OF:
        printf("AddrOf '%s'\n", node->as.addr_of.name);
        break;

    case NODE_TYPE_BIT:
        if (node->as.type_bit.width <= 1) {
            printf("TypeBit bit\n");
        } else {
            printf("TypeBit bit[%u]\n", node->as.type_bit.width);
        }
        break;

    case NODE_TYPE_NAMED:
        printf("TypeNamed '%s'\n", node->as.type_named.name);
        break;
    }
}
