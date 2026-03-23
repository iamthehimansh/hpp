#include "sema/symtab.h"
#include <string.h>

#define SCOPE_INIT_CAP 32

static unsigned int hash_str(const char *s)
{
    unsigned int h = 2166136261u;
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 16777619u;
    }
    return h;
}

static Scope *scope_create(Arena *arena, Scope *parent, int depth)
{
    Scope *s = arena_alloc(arena, sizeof(Scope));
    s->parent       = parent;
    s->depth        = depth;
    s->sym_capacity = SCOPE_INIT_CAP;
    s->sym_count    = 0;
    s->symbols      = arena_alloc(arena, (size_t)s->sym_capacity * sizeof(Symbol *));
    memset(s->symbols, 0, (size_t)s->sym_capacity * sizeof(Symbol *));
    return s;
}

static int scope_find_slot(Scope *s, const char *name)
{
    unsigned int h = hash_str(name);
    int idx = (int)(h & (unsigned int)(s->sym_capacity - 1));
    for (;;) {
        if (s->symbols[idx] == NULL) {
            return idx;
        }
        if (strcmp(s->symbols[idx]->name, name) == 0) {
            return idx;
        }
        idx = (idx + 1) & (s->sym_capacity - 1);
    }
}

static void scope_grow(Arena *arena, Scope *s)
{
    int new_cap = s->sym_capacity * 2;
    Symbol **new_syms = arena_alloc(arena, (size_t)new_cap * sizeof(Symbol *));
    memset(new_syms, 0, (size_t)new_cap * sizeof(Symbol *));

    for (int i = 0; i < s->sym_capacity; i++) {
        if (s->symbols[i] != NULL) {
            unsigned int h = hash_str(s->symbols[i]->name);
            int idx = (int)(h & (unsigned int)(new_cap - 1));
            while (new_syms[idx] != NULL) {
                idx = (idx + 1) & (new_cap - 1);
            }
            new_syms[idx] = s->symbols[i];
        }
    }
    s->symbols      = new_syms;
    s->sym_capacity = new_cap;
}

static Symbol *scope_lookup(Scope *s, const char *name)
{
    int slot = scope_find_slot(s, name);
    return s->symbols[slot];  /* NULL if not found */
}

/* --------------- public API --------------- */

SymTable *symtab_create(Arena *arena)
{
    SymTable *tab = arena_alloc(arena, sizeof(SymTable));
    tab->arena   = arena;
    tab->current = scope_create(arena, NULL, 0);
    return tab;
}

void symtab_enter_scope(SymTable *tab)
{
    Scope *child = scope_create(tab->arena, tab->current, tab->current->depth + 1);
    tab->current = child;
}

void symtab_exit_scope(SymTable *tab)
{
    if (tab->current->parent != NULL) {
        tab->current = tab->current->parent;
    }
}

Symbol *symtab_define(SymTable *tab, const char *name, SymKind kind,
                      HppType *type, SourceLoc loc)
{
    /* Check for duplicate in current scope */
    if (symtab_lookup_local(tab, name) != NULL) {
        return NULL;
    }

    Symbol *sym = arena_alloc(tab->arena, sizeof(Symbol));
    memset(sym, 0, sizeof(Symbol));
    sym->name        = name;
    sym->kind        = kind;
    sym->type        = type;
    sym->loc         = loc;
    sym->is_mutable  = false;
    sym->scope_depth = tab->current->depth;
    sym->reg_index   = -1;

    int slot = scope_find_slot(tab->current, name);
    tab->current->symbols[slot] = sym;
    tab->current->sym_count++;

    if (tab->current->sym_count * 2 > tab->current->sym_capacity) {
        scope_grow(tab->arena, tab->current);
    }

    return sym;
}

Symbol *symtab_lookup(SymTable *tab, const char *name)
{
    for (Scope *s = tab->current; s != NULL; s = s->parent) {
        Symbol *sym = scope_lookup(s, name);
        if (sym != NULL) {
            return sym;
        }
    }
    return NULL;
}

Symbol *symtab_lookup_local(SymTable *tab, const char *name)
{
    return scope_lookup(tab->current, name);
}

int symtab_depth(SymTable *tab)
{
    return tab->current->depth;
}
