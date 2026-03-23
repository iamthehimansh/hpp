// ast.c rewritten in H++
// AST node construction, list operations, and printing

import std/{sys, mem, str, io, fmt, printf};

def int  bit[32];
def long bit[64];

fn arena_alloc(arena: long, size: long) -> long;

// AstNode layout (80 bytes): kind@0, loc@8, resolved_type@24, as@32
const AN_KIND = 0;
const AN_LOC  = 8;
const AN_RTYPE = 24;
const AN_AS   = 32;
const AN_SIZE = 80;

// AstNodeList (16 bytes): node@0, next@8
const NL_NODE = 0;
const NL_NEXT = 8;
const NL_SIZE = 16;

// SourceLoc (16 bytes): filename@0, line@8, col@12
const SL_FILE = 0;
const SL_LINE = 8;
const SL_COL  = 12;
const SL_SIZE = 16;

// Node kinds (must match C enum)
enum AstNodeKind {
    NODE_PROGRAM, NODE_TYPE_DEF, NODE_FUNC_DEF, NODE_OPP_DEF,
    NODE_IMPORT, NODE_LINK,
    NODE_BLOCK, NODE_VAR_DECL, NODE_EXPR_STMT, NODE_RETURN_STMT,
    NODE_IF_STMT, NODE_WHILE_STMT, NODE_FOR_STMT,
    NODE_BREAK_STMT, NODE_CONTINUE_STMT, NODE_SWITCH_STMT, NODE_ASM_BLOCK,
    NODE_INT_LIT, NODE_BOOL_LIT, NODE_STRING_LIT, NODE_IDENT,
    NODE_BINARY_EXPR, NODE_UNARY_EXPR, NODE_CALL_EXPR,
    NODE_ASSIGN_EXPR, NODE_CAST_EXPR, NODE_ADDR_OF,
    NODE_DEREF, NODE_DEREF_ASSIGN,
    NODE_TYPE_BIT, NODE_TYPE_NAMED
}

fn ast_node_kind_str(kind: int) -> long {
    switch (kind) {
        case 0:  return "Program";
        case 1:  return "TypeDef";
        case 2:  return "FuncDef";
        case 3:  return "OppDef";
        case 4:  return "Import";
        case 5:  return "Link";
        case 6:  return "Block";
        case 7:  return "VarDecl";
        case 8:  return "ExprStmt";
        case 9:  return "ReturnStmt";
        case 10: return "IfStmt";
        case 11: return "WhileStmt";
        case 12: return "ForStmt";
        case 13: return "BreakStmt";
        case 14: return "ContinueStmt";
        case 15: return "SwitchStmt";
        case 16: return "AsmBlock";
        case 17: return "IntLit";
        case 18: return "BoolLit";
        case 19: return "StringLit";
        case 20: return "Ident";
        case 21: return "BinaryExpr";
        case 22: return "UnaryExpr";
        case 23: return "CallExpr";
        case 24: return "AssignExpr";
        case 25: return "CastExpr";
        case 26: return "AddrOf";
        case 27: return "Deref";
        case 28: return "DerefAssign";
        case 29: return "TypeBit";
        case 30: return "TypeNamed";
        default: return "Unknown";
    }
}

fn binop_str(op: int) -> long {
    switch (op) {
        case 0:  return "+";
        case 1:  return "-";
        case 2:  return "*";
        case 3:  return "/";
        case 4:  return "%";
        case 5:  return "&";
        case 6:  return "|";
        case 7:  return "^";
        case 8:  return "<<";
        case 9:  return ">>";
        case 10: return "==";
        case 11: return "!=";
        case 12: return "<";
        case 13: return ">";
        case 14: return "<=";
        case 15: return ">=";
        case 16: return "&&";
        case 17: return "||";
        default: return "?";
    }
}

fn unop_str(op: int) -> long {
    switch (op) {
        case 0: return "-";
        case 1: return "!";
        case 2: return "~";
        default: return "?";
    }
}

// SourceLoc is 16 bytes passed by value in 2 registers:
//   loc_a (rdx) = filename pointer (8 bytes)
//   loc_b (rcx) = line(4B) + col(4B) packed in 8 bytes
fn ast_new(arena: long, kind: int, loc_a: long, loc_b: long) -> long {
    long node = arena_alloc(arena, AN_SIZE);
    mem_write32(node, AN_KIND, kind);
    // Store SourceLoc at offset 8
    mem_write64(node, AN_LOC, loc_a);       // filename
    mem_write64(node, AN_LOC + 8, loc_b);   // line + col
    return node;
}

fn ast_list_append(arena: long, list: long, node: long) -> long {
    long entry = arena_alloc(arena, NL_SIZE);
    mem_write64(entry, NL_NODE, node);
    mem_write64(entry, NL_NEXT, 0);

    if (list == 0) {
        return entry;
    }

    // Walk to end
    long cur = list;
    long next = mem_read64(cur, NL_NEXT);
    while (next != 0) {
        cur = next;
        next = mem_read64(cur, NL_NEXT);
    }
    mem_write64(cur, NL_NEXT, entry);
    return list;
}

fn ast_list_len(list: long) -> int {
    int count = 0;
    long cur = list;
    while (cur != 0) {
        count++;
        cur = mem_read64(cur, NL_NEXT);
    }
    return count;
}

fn print_indent(n: int) {
    for (int i = 0; i < n; i++) {
        print_char(' ');
    }
}

fn ast_print(node: long, indent: int) {
    if (node == 0) { return; }
    print_indent(indent);

    int kind = mem_read32(node, AN_KIND);
    long name = ast_node_kind_str(kind);

    // Most nodes: print kind name + details, then recurse on children
    // For simplicity, just print the kind for all nodes
    // The C version has elaborate per-node printing — we match the format
    switch (kind) {
        case 0: {
            // NODE_PROGRAM
            puts("Program\n");
            long n = mem_read64(node, AN_AS);  // program.decls (first field of union)
            while (n != 0) {
                long child = mem_read64(n, NL_NODE);
                ast_print(child, indent + 2);
                n = mem_read64(n, NL_NEXT);
            }
            break;
        }
        case 1: {
            // NODE_TYPE_DEF: name@32(8B), bit_width@40(4B)
            long tname = mem_read64(node, AN_AS);
            int bw = mem_read32(node, AN_AS + 8);
            fmt_out("TypeDef '%s' = bit[%d]\n", tname, bw, 0, 0);
            break;
        }
        case 2: {
            // NODE_FUNC_DEF: name@32(8B)
            long fname = mem_read64(node, AN_AS);
            fmt_out("FuncDef '%s'\n", fname, 0, 0, 0);
            // Print return type if present
            long ret_type = mem_read64(node, AN_AS + 24);  // return_type
            if (ret_type != 0) {
                ast_print(ret_type, indent + 2);
            }
            // Print body
            long body = mem_read64(node, AN_AS + 32);  // body
            if (body != 0) {
                ast_print(body, indent + 2);
            }
            break;
        }
        default: {
            fmt_out("%s\n", name, 0, 0, 0);
            break;
        }
    }
}
