#include "codegen/codegen.h"
#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

/* ---- String literal table ---- */

typedef struct {
    const char *text;
    size_t len;
    int id;     /* label: .str<id> */
} StringEntry;

static StringEntry g_strings[1024];
static int g_string_count = 0;

static int add_string_literal(const char *text, size_t len) {
    /* Check for duplicate */
    for (int i = 0; i < g_string_count; i++) {
        if (g_strings[i].len == len && memcmp(g_strings[i].text, text, len) == 0) {
            return g_strings[i].id;
        }
    }
    int id = g_string_count;
    g_strings[g_string_count].text = text;
    g_strings[g_string_count].len = len;
    g_strings[g_string_count].id = id;
    g_string_count++;
    return id;
}

/* ---- Variable map (per-function, maps name -> stack offset) ---- */

typedef struct {
    const char *name;   /* interned pointer */
    int offset;         /* positive offset from rbp (access as [rbp - offset]) */
    int bits;           /* actual bit width of the variable */
} VarMapEntry;

#define VAR_MAP_MAX 256

typedef struct {
    VarMapEntry entries[VAR_MAP_MAX];
    int count;
    int next_offset;
} VarMap;

static void var_map_reset(VarMap *vm) {
    vm->count = 0;
    vm->next_offset = 0;
}

static void var_map_add(VarMap *vm, const char *name, int bits) {
    if (vm->count >= VAR_MAP_MAX) {
        return; /* silently cap -- should not happen in practice */
    }
    vm->next_offset += 8; /* every slot is 8 bytes */
    vm->entries[vm->count].name = name;
    vm->entries[vm->count].offset = vm->next_offset;
    vm->entries[vm->count].bits = bits;
    vm->count++;
}

static VarMapEntry *var_map_find(VarMap *vm, const char *name) {
    for (int i = vm->count - 1; i >= 0; i--) {
        if (vm->entries[i].name == name) { /* interned pointer compare */
            return &vm->entries[i];
        }
    }
    return NULL;
}

/* ---- Output helpers ---- */

static void emit(CodeGen *cg, const char *fmt, ...) {
    va_list ap;
    for (;;) {
        size_t avail = cg->output_cap - cg->output_len;
        va_start(ap, fmt);
        int n = vsnprintf(cg->output + cg->output_len, avail, fmt, ap);
        va_end(ap);
        if (n < 0) {
            return;
        }
        if ((size_t)n < avail) {
            cg->output_len += (size_t)n;
            return;
        }
        /* grow buffer */
        size_t new_cap = cg->output_cap * 2;
        if (new_cap < cg->output_len + (size_t)n + 1) {
            new_cap = cg->output_len + (size_t)n + 64;
        }
        char *new_buf = realloc(cg->output, new_cap);
        if (!new_buf) {
            return;
        }
        cg->output = new_buf;
        cg->output_cap = new_cap;
    }
}

static void emit_label(CodeGen *cg, int id) {
    emit(cg, ".L%d:\n", id);
}

static int new_label(CodeGen *cg) {
    return cg->label_counter++;
}

/* ---- Register name helpers ---- */

static const char *reg_a(int bits) {
    switch (bits) {
        case 8:  return "al";
        case 16: return "ax";
        case 32: return "eax";
        case 64: return "rax";
        default: return "rax";
    }
}

static const char *reg_c(int bits) {
    switch (bits) {
        case 8:  return "cl";
        case 16: return "cx";
        case 32: return "ecx";
        case 64: return "rcx";
        default: return "rcx";
    }
}

/* 64-bit register name for a given arg index (for push/pop) */
static const char *arg_reg64(int index) {
    switch (index) {
        case 0: return "rdi";
        case 1: return "rsi";
        case 2: return "rdx";
        case 3: return "rcx";
        case 4: return "r8";
        case 5: return "r9";
        default: return "rdi";
    }
}

/* ---- Forward declarations ---- */

static VarMap g_var_map;

static void gen_block(CodeGen *cg, AstNode *block);
static void gen_stmt(CodeGen *cg, AstNode *node);
static void gen_expr(CodeGen *cg, AstNode *node);

/* ---- Pre-scan: assign stack offsets to all locals ---- */

static void prescan_vars(CodeGen *cg, AstNode *node) {
    if (!node) {
        return;
    }
    (void)cg;

    switch (node->kind) {
        case NODE_VAR_DECL: {
            int bits = 64; /* default slot width */
            if (node->resolved_type) {
                int rs = type_reg_size(node->resolved_type->bit_width);
                if (rs > 0) {
                    bits = rs;
                }
            }
            var_map_add(&g_var_map, node->as.var_decl.name, bits);
            break;
        }
        case NODE_BLOCK: {
            AstNodeList *it = node->as.block.stmts;
            while (it) {
                prescan_vars(cg, it->node);
                it = it->next;
            }
            break;
        }
        case NODE_IF_STMT:
            prescan_vars(cg, node->as.if_stmt.then_block);
            prescan_vars(cg, node->as.if_stmt.else_block);
            break;
        case NODE_WHILE_STMT:
            prescan_vars(cg, node->as.while_stmt.body);
            break;
        case NODE_FOR_STMT:
            prescan_vars(cg, node->as.for_stmt.init);
            prescan_vars(cg, node->as.for_stmt.body);
            break;
        case NODE_SWITCH_STMT:
            for (int i = 0; i < node->as.switch_stmt.case_count; i++) {
                prescan_vars(cg, node->as.switch_stmt.case_bodies[i]);
            }
            prescan_vars(cg, node->as.switch_stmt.default_body);
            break;
        default:
            break;
    }
}

/* ---- Align up to a multiple of 'align' ---- */

static int align_up(int value, int align) {
    return (value + align - 1) & ~(align - 1);
}

/* ---- Expression codegen (result always in rax, properly sized) ---- */

static void gen_int_lit(CodeGen *cg, AstNode *node) {
    int bits = 64;
    if (node->resolved_type) {
        int rs = type_reg_size(node->resolved_type->bit_width);
        if (rs > 0) {
            bits = rs;
        }
    }
    emit(cg, "    mov %s, %" PRIu64 "\n", reg_a(bits), node->as.int_lit.value);
}

static void gen_bool_lit(CodeGen *cg, AstNode *node) {
    emit(cg, "    mov al, %d\n", node->as.bool_lit.value ? 1 : 0);
}

static void gen_string_lit(CodeGen *cg, AstNode *node) {
    int id = add_string_literal(node->as.string_lit.text, node->as.string_lit.len);
    node->as.string_lit.data_id = id;
    emit(cg, "    lea rax, [rel __str%d]\n", id);
}

static void gen_ident_expr(CodeGen *cg, AstNode *node) {
    VarMapEntry *v = var_map_find(&g_var_map, node->as.ident.name);
    if (!v) {
        error_report(node->loc, ERR_CODEGEN,
                     "undefined variable '%s' in codegen", node->as.ident.name);
        return;
    }
    int bits = v->bits;
    switch (bits) {
        case 8:
            emit(cg, "    movzx eax, byte [rbp - %d]\n", v->offset);
            break;
        case 16:
            emit(cg, "    movzx eax, word [rbp - %d]\n", v->offset);
            break;
        case 32:
            emit(cg, "    mov eax, dword [rbp - %d]\n", v->offset);
            break;
        case 64:
        default:
            emit(cg, "    mov rax, qword [rbp - %d]\n", v->offset);
            break;
    }
}

static void gen_assign_expr(CodeGen *cg, AstNode *node) {
    gen_expr(cg, node->as.assign.value);
    VarMapEntry *v = var_map_find(&g_var_map, node->as.assign.name);
    if (!v) {
        error_report(node->loc, ERR_CODEGEN,
                     "undefined variable '%s' in codegen", node->as.assign.name);
        return;
    }
    emit(cg, "    mov qword [rbp - %d], rax\n", v->offset);
}

static void gen_binary_expr(CodeGen *cg, AstNode *node) {
    BinOp op = node->as.binary.op;
    int bits = 64;

    /* For comparisons, the result type is 'bit' but we need the operand size */
    if (op >= BINOP_EQ && op <= BINOP_GE) {
        /* Use left operand's type for register sizing */
        if (node->as.binary.left->resolved_type) {
            int rs = type_reg_size(node->as.binary.left->resolved_type->bit_width);
            if (rs > 0) bits = rs;
        }
    } else if (node->resolved_type) {
        int rs = type_reg_size(node->resolved_type->bit_width);
        if (rs > 0) bits = rs;
    }

    /* Short-circuit logic operators */
    if (op == BINOP_LOGIC_AND) {
        int lbl_short = new_label(cg);
        gen_expr(cg, node->as.binary.left);
        emit(cg, "    test al, al\n");
        emit(cg, "    jz .L%d\n", lbl_short);
        gen_expr(cg, node->as.binary.right);
        emit_label(cg, lbl_short);
        return;
    }
    if (op == BINOP_LOGIC_OR) {
        int lbl_short = new_label(cg);
        gen_expr(cg, node->as.binary.left);
        emit(cg, "    test al, al\n");
        emit(cg, "    jnz .L%d\n", lbl_short);
        gen_expr(cg, node->as.binary.right);
        emit_label(cg, lbl_short);
        return;
    }

    /* General binary: left in rax, right in rcx */
    gen_expr(cg, node->as.binary.left);
    emit(cg, "    push rax\n");
    gen_expr(cg, node->as.binary.right);
    emit(cg, "    mov %s, %s\n", reg_c(bits), reg_a(bits));
    emit(cg, "    pop rax\n");

    switch (op) {
        case BINOP_ADD:
            emit(cg, "    add %s, %s\n", reg_a(bits), reg_c(bits));
            break;
        case BINOP_SUB:
            emit(cg, "    sub %s, %s\n", reg_a(bits), reg_c(bits));
            break;
        case BINOP_MUL:
            if (bits == 8) {
                emit(cg, "    movzx eax, al\n");
                emit(cg, "    movzx ecx, cl\n");
                emit(cg, "    imul eax, ecx\n");
            } else {
                emit(cg, "    imul %s, %s\n", reg_a(bits), reg_c(bits));
            }
            break;
        case BINOP_DIV:
            if (bits == 8) {
                emit(cg, "    movzx ax, al\n");
                emit(cg, "    div cl\n");
                /* quotient in al */
            } else if (bits == 16) {
                emit(cg, "    xor dx, dx\n");
                emit(cg, "    div cx\n");
            } else if (bits == 32) {
                emit(cg, "    xor edx, edx\n");
                emit(cg, "    div ecx\n");
            } else {
                emit(cg, "    xor rdx, rdx\n");
                emit(cg, "    div rcx\n");
            }
            break;
        case BINOP_MOD:
            if (bits == 8) {
                emit(cg, "    movzx ax, al\n");
                emit(cg, "    div cl\n");
                emit(cg, "    mov al, ah\n"); /* remainder in ah */
            } else if (bits == 16) {
                emit(cg, "    xor dx, dx\n");
                emit(cg, "    div cx\n");
                emit(cg, "    mov ax, dx\n");
            } else if (bits == 32) {
                emit(cg, "    xor edx, edx\n");
                emit(cg, "    div ecx\n");
                emit(cg, "    mov eax, edx\n");
            } else {
                emit(cg, "    xor rdx, rdx\n");
                emit(cg, "    div rcx\n");
                emit(cg, "    mov rax, rdx\n");
            }
            break;
        case BINOP_BIT_AND:
            emit(cg, "    and %s, %s\n", reg_a(bits), reg_c(bits));
            break;
        case BINOP_BIT_OR:
            emit(cg, "    or %s, %s\n", reg_a(bits), reg_c(bits));
            break;
        case BINOP_BIT_XOR:
            emit(cg, "    xor %s, %s\n", reg_a(bits), reg_c(bits));
            break;
        case BINOP_SHL:
            emit(cg, "    shl %s, cl\n", reg_a(bits));
            break;
        case BINOP_SHR:
            emit(cg, "    shr %s, cl\n", reg_a(bits));
            break;
        case BINOP_EQ:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    sete al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_NE:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    setne al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_LT:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    setb al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_GT:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    seta al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_LE:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    setbe al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_GE:
            emit(cg, "    cmp %s, %s\n", reg_a(bits), reg_c(bits));
            emit(cg, "    setae al\n");
            emit(cg, "    movzx eax, al\n");
            break;
        case BINOP_LOGIC_AND:
        case BINOP_LOGIC_OR:
            /* handled above */
            break;
    }
}

static void gen_unary_expr(CodeGen *cg, AstNode *node) {
    gen_expr(cg, node->as.unary.operand);
    int bits = 64;
    if (node->resolved_type) {
        int rs = type_reg_size(node->resolved_type->bit_width);
        if (rs > 0) {
            bits = rs;
        }
    }
    switch (node->as.unary.op) {
        case UNOP_NEG:
            emit(cg, "    neg %s\n", reg_a(bits));
            break;
        case UNOP_NOT:
            emit(cg, "    xor al, 1\n");
            break;
        case UNOP_BITNOT:
            emit(cg, "    not %s\n", reg_a(bits));
            break;
    }
}

static void gen_call_expr(CodeGen *cg, AstNode *node) {
    int argc = node->as.call.arg_count;

    /* Check if this is an indirect call (function pointer in a variable) */
    VarMapEntry *fptr = var_map_find(&g_var_map, node->as.call.func_name);

    /* Evaluate all arguments left to right, push each */
    for (int i = 0; i < argc; i++) {
        gen_expr(cg, node->as.call.args[i]);
        emit(cg, "    push rax\n");
    }

    /* Pop into correct registers in reverse order */
    for (int i = argc - 1; i >= 0; i--) {
        emit(cg, "    pop %s\n", arg_reg64(i));
    }

    if (fptr) {
        /* Indirect call: load fn ptr into r11 (caller-saved scratch) and call */
        emit(cg, "    mov r11, qword [rbp - %d]\n", fptr->offset);
        emit(cg, "    call r11\n");
    } else {
        /* Direct call */
        emit(cg, "    call %s\n", node->as.call.func_name);
    }
    /* Result is in rax */
}

static void gen_cast_expr(CodeGen *cg, AstNode *node) {
    gen_expr(cg, node->as.cast.expr);
    int src = 64, dst = 64;
    if (node->as.cast.expr->resolved_type)
        src = type_reg_size(node->as.cast.expr->resolved_type->bit_width);
    if (node->resolved_type)
        dst = type_reg_size(node->resolved_type->bit_width);
    if (src == dst) return;
    /* Narrowing: truncate to target width */
    if (dst == 8)       emit(cg, "    movzx eax, al\n");
    else if (dst == 16) emit(cg, "    movzx eax, ax\n");
    else if (dst == 32) emit(cg, "    mov eax, eax\n");
    /* Widening: zero-extend to 64-bit */
    else if (dst == 64 && src == 8)  emit(cg, "    movzx rax, al\n");
    else if (dst == 64 && src == 16) emit(cg, "    movzx rax, ax\n");
    else if (dst == 64 && src == 32) emit(cg, "    mov eax, eax\n");
}

static void gen_expr(CodeGen *cg, AstNode *node) {
    if (!node) {
        return;
    }
    switch (node->kind) {
        case NODE_INT_LIT:     gen_int_lit(cg, node);     break;
        case NODE_BOOL_LIT:    gen_bool_lit(cg, node);    break;
        case NODE_STRING_LIT:  gen_string_lit(cg, node);  break;
        case NODE_IDENT:       gen_ident_expr(cg, node);  break;
        case NODE_ASSIGN_EXPR: gen_assign_expr(cg, node); break;
        case NODE_BINARY_EXPR: gen_binary_expr(cg, node); break;
        case NODE_UNARY_EXPR:  gen_unary_expr(cg, node);  break;
        case NODE_CALL_EXPR:   gen_call_expr(cg, node);   break;
        case NODE_CAST_EXPR:   gen_cast_expr(cg, node);   break;
        case NODE_ADDR_OF: {
            VarMapEntry *v = var_map_find(&g_var_map, node->as.addr_of.name);
            if (v) {
                emit(cg, "    lea rax, [rbp - %d]\n", v->offset);
            } else {
                /* Assume it's a function name — get its address */
                emit(cg, "    lea rax, [rel %s]\n", node->as.addr_of.name);
            }
            break;
        }
        case NODE_DEREF:
            /* *ptr — read 64-bit value from address */
            gen_expr(cg, node->as.deref.operand);  /* addr in rax */
            emit(cg, "    mov rax, [rax]\n");       /* load value */
            break;
        case NODE_DEREF_ASSIGN:
            /* *ptr = val — write 64-bit value to address */
            gen_expr(cg, node->as.deref_assign.value);  /* val in rax */
            emit(cg, "    push rax\n");
            gen_expr(cg, node->as.deref_assign.addr);   /* addr in rax */
            emit(cg, "    pop rcx\n");                   /* val in rcx */
            emit(cg, "    mov [rax], rcx\n");            /* store */
            emit(cg, "    mov rax, rcx\n");              /* return val */
            break;
        default:
            error_report(node->loc, ERR_CODEGEN,
                         "unexpected node kind '%s' in expression",
                         ast_node_kind_str(node->kind));
            break;
    }
}

/* ---- Statement codegen ---- */

static void gen_var_decl(CodeGen *cg, AstNode *node) {
    VarMapEntry *v = var_map_find(&g_var_map, node->as.var_decl.name);
    if (!v) {
        error_report(node->loc, ERR_CODEGEN,
                     "variable '%s' not found in var map", node->as.var_decl.name);
        return;
    }
    if (node->as.var_decl.init_expr) {
        gen_expr(cg, node->as.var_decl.init_expr);
        emit(cg, "    mov qword [rbp - %d], rax\n", v->offset);
    } else {
        emit(cg, "    mov qword [rbp - %d], 0\n", v->offset);
    }
}

static void gen_return_stmt(CodeGen *cg, AstNode *node) {
    if (node->as.return_stmt.expr) {
        gen_expr(cg, node->as.return_stmt.expr);
    }
    emit(cg, "    mov rsp, rbp\n");
    emit(cg, "    pop rbp\n");
    emit(cg, "    ret\n");
}

static void gen_if_stmt(CodeGen *cg, AstNode *node) {
    int lbl_else = new_label(cg);
    int lbl_end  = new_label(cg);

    gen_expr(cg, node->as.if_stmt.condition);
    emit(cg, "    test al, al\n");
    emit(cg, "    jz .L%d\n", lbl_else);

    gen_block(cg, node->as.if_stmt.then_block);
    if (node->as.if_stmt.else_block) {
        emit(cg, "    jmp .L%d\n", lbl_end);
    }

    emit_label(cg, lbl_else);
    if (node->as.if_stmt.else_block) {
        gen_block(cg, node->as.if_stmt.else_block);
        emit_label(cg, lbl_end);
    }
}

static void gen_while_stmt(CodeGen *cg, AstNode *node) {
    int lbl_loop = new_label(cg);
    int lbl_end  = new_label(cg);

    LoopCtx ctx;
    ctx.break_label    = lbl_end;
    ctx.continue_label = lbl_loop;
    ctx.parent         = cg->loop_ctx;
    cg->loop_ctx       = &ctx;

    emit_label(cg, lbl_loop);
    gen_expr(cg, node->as.while_stmt.condition);
    emit(cg, "    test al, al\n");
    emit(cg, "    jz .L%d\n", lbl_end);
    gen_block(cg, node->as.while_stmt.body);
    emit(cg, "    jmp .L%d\n", lbl_loop);
    emit_label(cg, lbl_end);

    cg->loop_ctx = ctx.parent;
}

static void gen_for_stmt(CodeGen *cg, AstNode *node) {
    int lbl_loop     = new_label(cg);
    int lbl_continue = new_label(cg);
    int lbl_end      = new_label(cg);

    LoopCtx ctx;
    ctx.break_label    = lbl_end;
    ctx.continue_label = lbl_continue;
    ctx.parent         = cg->loop_ctx;
    cg->loop_ctx       = &ctx;

    /* Init */
    if (node->as.for_stmt.init) {
        gen_stmt(cg, node->as.for_stmt.init);
    }

    emit_label(cg, lbl_loop);

    /* Condition */
    if (node->as.for_stmt.condition) {
        gen_expr(cg, node->as.for_stmt.condition);
        emit(cg, "    test al, al\n");
        emit(cg, "    jz .L%d\n", lbl_end);
    }

    /* Body */
    gen_block(cg, node->as.for_stmt.body);

    /* Continue label and update */
    emit_label(cg, lbl_continue);
    if (node->as.for_stmt.update) {
        gen_expr(cg, node->as.for_stmt.update);
    }
    emit(cg, "    jmp .L%d\n", lbl_loop);

    emit_label(cg, lbl_end);

    cg->loop_ctx = ctx.parent;
}

static void gen_break_stmt(CodeGen *cg, AstNode *node) {
    if (!cg->loop_ctx) {
        error_report(node->loc, ERR_CODEGEN, "break outside of loop");
        return;
    }
    emit(cg, "    jmp .L%d\n", cg->loop_ctx->break_label);
}

static void gen_continue_stmt(CodeGen *cg, AstNode *node) {
    if (!cg->loop_ctx) {
        error_report(node->loc, ERR_CODEGEN, "continue outside of loop");
        return;
    }
    emit(cg, "    jmp .L%d\n", cg->loop_ctx->continue_label);
}

static void gen_asm_block(CodeGen *cg, AstNode *node) {
    /* Only emit if target matches (linux for now) */
    if (node->as.asm_block.target == ASM_TARGET_LINUX) {
        /* Emit raw assembly body */
        if (node->as.asm_block.body && node->as.asm_block.body_len > 0) {
            emit(cg, "%.*s\n", node->as.asm_block.body_len,
                 node->as.asm_block.body);
        }
    }
}

static void gen_stmt(CodeGen *cg, AstNode *node) {
    if (!node) {
        return;
    }
    switch (node->kind) {
        case NODE_VAR_DECL:      gen_var_decl(cg, node);      break;
        case NODE_RETURN_STMT:   gen_return_stmt(cg, node);   break;
        case NODE_IF_STMT:       gen_if_stmt(cg, node);       break;
        case NODE_WHILE_STMT:    gen_while_stmt(cg, node);    break;
        case NODE_FOR_STMT:      gen_for_stmt(cg, node);      break;
        case NODE_BREAK_STMT:    gen_break_stmt(cg, node);    break;
        case NODE_CONTINUE_STMT: gen_continue_stmt(cg, node); break;
        case NODE_SWITCH_STMT: {
            gen_expr(cg, node->as.switch_stmt.expr);
            int end_label = new_label(cg);
            int *case_labels = arena_alloc(cg->arena,
                (size_t)node->as.switch_stmt.case_count * sizeof(int));
            int default_label = new_label(cg);

            /* Push break context so break jumps to end_label */
            LoopCtx sw_ctx;
            sw_ctx.break_label = end_label;
            sw_ctx.continue_label = -1;
            sw_ctx.parent = cg->loop_ctx;
            cg->loop_ctx = &sw_ctx;

            /* Use correct register size for comparison */
            int sw_bits = 64;
            if (node->as.switch_stmt.expr->resolved_type) {
                int rs = type_reg_size(node->as.switch_stmt.expr->resolved_type->bit_width);
                if (rs > 0) sw_bits = rs;
            }
            for (int i = 0; i < node->as.switch_stmt.case_count; i++) {
                case_labels[i] = new_label(cg);
                emit(cg, "    cmp %s, %" PRIu64 "\n",
                     reg_a(sw_bits), node->as.switch_stmt.case_values[i]);
                emit(cg, "    je .L%d\n", case_labels[i]);
            }
            if (node->as.switch_stmt.default_body) {
                emit(cg, "    jmp .L%d\n", default_label);
            } else {
                emit(cg, "    jmp .L%d\n", end_label);
            }

            for (int i = 0; i < node->as.switch_stmt.case_count; i++) {
                emit_label(cg, case_labels[i]);
                gen_block(cg, node->as.switch_stmt.case_bodies[i]);
            }

            if (node->as.switch_stmt.default_body) {
                emit_label(cg, default_label);
                gen_block(cg, node->as.switch_stmt.default_body);
            }

            cg->loop_ctx = sw_ctx.parent;
            emit_label(cg, end_label);
            break;
        }
        case NODE_ASM_BLOCK:     gen_asm_block(cg, node);     break;
        case NODE_EXPR_STMT:
            gen_expr(cg, node->as.expr_stmt.expr);
            break;
        case NODE_BLOCK:
            gen_block(cg, node);
            break;
        default:
            error_report(node->loc, ERR_CODEGEN,
                         "unexpected node kind '%s' in statement",
                         ast_node_kind_str(node->kind));
            break;
    }
}

static void gen_block(CodeGen *cg, AstNode *block) {
    if (!block || block->kind != NODE_BLOCK) {
        return;
    }
    AstNodeList *it = block->as.block.stmts;
    while (it) {
        gen_stmt(cg, it->node);
        it = it->next;
    }
}

/* ---- Function definition ---- */

static void gen_func_def(CodeGen *cg, AstNode *node) {
    const char *name = node->as.func_def.name;
    cg->current_func_name = name;

    /* Reset variable map */
    var_map_reset(&g_var_map);

    /* Add parameters to var map */
    ParamList *pl = &node->as.func_def.params;
    for (int i = 0; i < pl->count; i++) {
        int bits = 64; /* default */
        Param *p = &pl->params[i];
        if (p->type_node && p->type_node->resolved_type) {
            int rs = type_reg_size(p->type_node->resolved_type->bit_width);
            if (rs > 0) {
                bits = rs;
            }
        }
        var_map_add(&g_var_map, p->name, bits);
    }

    /* Pre-scan body for variable declarations */
    prescan_vars(cg, node->as.func_def.body);

    /* Calculate frame size (aligned to 16) */
    int frame_size = align_up(g_var_map.next_offset, 16);

    /* Emit function label */
    emit(cg, "%s:\n", name);

    /* Prologue */
    emit(cg, "    push rbp\n");
    emit(cg, "    mov rbp, rsp\n");
    if (frame_size > 0) {
        emit(cg, "    sub rsp, %d\n", frame_size);
    }

    /* Spill parameters from registers to stack slots */
    for (int i = 0; i < pl->count && i < 6; i++) {
        VarMapEntry *v = var_map_find(&g_var_map, pl->params[i].name);
        if (v) {
            emit(cg, "    mov qword [rbp - %d], %s\n",
                 v->offset, arg_reg64(i));
        }
    }

    /* Generate body */
    gen_block(cg, node->as.func_def.body);

    /* Fallthrough epilogue (in case function doesn't end with return) */
    emit(cg, "    mov rsp, rbp\n");
    emit(cg, "    pop rbp\n");
    emit(cg, "    ret\n\n");

    cg->current_func_name = NULL;
}

/* ---- Collect external function names ---- */

static const char *g_extern_names[256];
static int g_extern_count;

static void collect_externs(CodeGen *cg, AstNode *node) {
    if (!node) {
        return;
    }
    switch (node->kind) {
        case NODE_CALL_EXPR: {
            Symbol *sym = symtab_lookup(cg->symbols, node->as.call.func_name);
            if (sym && sym->is_external) {
                /* Check for duplicates */
                bool found = false;
                for (int i = 0; i < g_extern_count; i++) {
                    if (g_extern_names[i] == sym->name) {
                        found = true;
                        break;
                    }
                }
                if (!found && g_extern_count < 256) {
                    g_extern_names[g_extern_count++] = sym->name;
                }
            }
            /* Recurse into arguments */
            for (int i = 0; i < node->as.call.arg_count; i++) {
                collect_externs(cg, node->as.call.args[i]);
            }
            break;
        }
        case NODE_PROGRAM: {
            AstNodeList *it = node->as.program.decls;
            while (it) {
                collect_externs(cg, it->node);
                it = it->next;
            }
            break;
        }
        case NODE_FUNC_DEF:
            collect_externs(cg, node->as.func_def.body);
            break;
        case NODE_BLOCK: {
            AstNodeList *it = node->as.block.stmts;
            while (it) {
                collect_externs(cg, it->node);
                it = it->next;
            }
            break;
        }
        case NODE_VAR_DECL:
            collect_externs(cg, node->as.var_decl.init_expr);
            break;
        case NODE_EXPR_STMT:
            collect_externs(cg, node->as.expr_stmt.expr);
            break;
        case NODE_RETURN_STMT:
            collect_externs(cg, node->as.return_stmt.expr);
            break;
        case NODE_IF_STMT:
            collect_externs(cg, node->as.if_stmt.condition);
            collect_externs(cg, node->as.if_stmt.then_block);
            collect_externs(cg, node->as.if_stmt.else_block);
            break;
        case NODE_WHILE_STMT:
            collect_externs(cg, node->as.while_stmt.condition);
            collect_externs(cg, node->as.while_stmt.body);
            break;
        case NODE_FOR_STMT:
            collect_externs(cg, node->as.for_stmt.init);
            collect_externs(cg, node->as.for_stmt.condition);
            collect_externs(cg, node->as.for_stmt.update);
            collect_externs(cg, node->as.for_stmt.body);
            break;
        case NODE_SWITCH_STMT:
            collect_externs(cg, node->as.switch_stmt.expr);
            for (int i = 0; i < node->as.switch_stmt.case_count; i++) {
                collect_externs(cg, node->as.switch_stmt.case_bodies[i]);
            }
            collect_externs(cg, node->as.switch_stmt.default_body);
            break;
        case NODE_BINARY_EXPR:
            collect_externs(cg, node->as.binary.left);
            collect_externs(cg, node->as.binary.right);
            break;
        case NODE_UNARY_EXPR:
            collect_externs(cg, node->as.unary.operand);
            break;
        case NODE_ASSIGN_EXPR:
            collect_externs(cg, node->as.assign.value);
            break;
        case NODE_CAST_EXPR:
            collect_externs(cg, node->as.cast.expr);
            break;
        case NODE_INT_LIT:
        case NODE_BOOL_LIT:
        case NODE_IDENT:
        case NODE_BREAK_STMT:
        case NODE_CONTINUE_STMT:
        case NODE_ASM_BLOCK:
        case NODE_TYPE_DEF:
        case NODE_OPP_DEF:
        case NODE_IMPORT:
        case NODE_LINK:
        case NODE_STRING_LIT:
        case NODE_ADDR_OF:
            break;
        case NODE_DEREF:
            collect_externs(cg, node->as.deref.operand);
            break;
        case NODE_DEREF_ASSIGN:
            collect_externs(cg, node->as.deref_assign.addr);
            collect_externs(cg, node->as.deref_assign.value);
            break;
        case NODE_TYPE_BIT:
        case NODE_TYPE_NAMED:
            break;
    }
}

/* ---- Program generation ---- */

static void gen_program(CodeGen *cg, AstNode *program) {
    /* Reset state */
    g_extern_count = 0;
    g_string_count = 0;
    collect_externs(cg, program);

    /* Emit extern declarations */
    for (int i = 0; i < g_extern_count; i++) {
        emit(cg, "extern %s\n", g_extern_names[i]);
    }
    if (g_extern_count > 0) {
        emit(cg, "\n");
    }

    /* Emit text section header */
    emit(cg, "section .text\n");

    if (cg->is_library) {
        /* Library mode: export all functions as global, no _start */
        AstNodeList *pre = program->as.program.decls;
        while (pre) {
            if (pre->node && pre->node->kind == NODE_FUNC_DEF
                && pre->node->as.func_def.body != NULL) {
                emit(cg, "global %s\n", pre->node->as.func_def.name);
            }
            pre = pre->next;
        }
        emit(cg, "\n");
    } else {
        /* Executable mode: emit _start entry point */
        emit(cg, "global _start\n\n");
        emit(cg, "_start:\n");
        emit(cg, "    mov rdi, [rsp]\n");       /* argc */
        emit(cg, "    lea rsi, [rsp + 8]\n");   /* argv pointer */
        emit(cg, "    call main\n");
        emit(cg, "    mov rdi, rax\n");
        emit(cg, "    mov rax, 60\n");
        emit(cg, "    syscall\n\n");
    }

    /* Emit all function definitions */
    AstNodeList *it = program->as.program.decls;
    while (it) {
        if (it->node && it->node->kind == NODE_FUNC_DEF
            && it->node->as.func_def.body != NULL) {
            gen_func_def(cg, it->node);
        }
        it = it->next;
    }

    /* Emit string literals in .data section */
    if (g_string_count > 0) {
        emit(cg, "\nsection .data\n");
        for (int i = 0; i < g_string_count; i++) {
            emit(cg, "__str%d: db ", g_strings[i].id);
            for (size_t j = 0; j < g_strings[i].len; j++) {
                if (j > 0) emit(cg, ", ");
                emit(cg, "%d", (unsigned char)g_strings[i].text[j]);
            }
            emit(cg, ", 0\n");  /* null terminator */
        }
    }
}

/* ---- Public API ---- */

void codegen_init(CodeGen *cg, Arena *arena, TypeTable *types,
                  SymTable *symbols, OppTable *opps, const char *target) {
    cg->arena          = arena;
    cg->types          = types;
    cg->symbols        = symbols;
    cg->opps           = opps;
    cg->target         = target;
    cg->output_cap     = 4096;
    cg->output         = malloc(cg->output_cap);
    cg->output_len     = 0;
    cg->label_counter  = 0;
    cg->stack_size     = 0;
    cg->loop_ctx       = NULL;
    cg->current_func_name = NULL;
    if (cg->output) {
        cg->output[0] = '\0';
    }
}

char *codegen_generate(CodeGen *cg, AstNode *program) {
    if (!program || program->kind != NODE_PROGRAM) {
        error_report(program ? program->loc : (SourceLoc){NULL, 0, 0},
                     ERR_CODEGEN, "expected program node");
        return NULL;
    }
    gen_program(cg, program);
    return cg->output;
}
