#ifndef HPP_MODULE_H
#define HPP_MODULE_H

#include "common/arena.h"
#include "common/strtab.h"
#include "common/error.h"
#include "ast/ast.h"
#include <stdbool.h>

#define MAX_MODULES 64
#define MAX_SEARCH_PATHS 8

/* A resolved module: its .hdef declarations and implementation path.
   Implementation can be any ONE of: .asm, .hpp, or pre-compiled .o
   The .hdef always provides the type declarations. */
typedef struct {
    const char *name;           /* e.g., "std/io" or "mathlib" */
    const char *hdef_path;      /* path to .hdef file (type declarations) */
    const char *asm_path;       /* path to .asm file (may be NULL) */
    const char *hpp_path;       /* path to .hpp file (may be NULL) */
    const char *prebuilt_obj;   /* path to pre-compiled .o file (may be NULL) */
    const char *obj_path;       /* path to output .o (compiled from .asm/.hpp) */
    AstNode *declarations;      /* parsed declarations from .hdef */
    bool loaded;
} Module;

typedef struct {
    Arena *arena;
    StrTab *strtab;
    const char *search_paths[MAX_SEARCH_PATHS];
    int search_path_count;
    Module modules[MAX_MODULES];
    int module_count;
    /* External .o files from link directives */
    const char *link_objs[MAX_MODULES];
    int link_obj_count;
} ModuleSystem;

/* Create the module system */
ModuleSystem *module_system_create(Arena *arena, StrTab *strtab);

/* Add a search path (stdlib dir, local dir, etc.) */
void module_add_search_path(ModuleSystem *ms, const char *path);

/* Resolve and load a module by path (e.g., "std/io").
   Returns the Module or NULL on failure. */
Module *module_resolve(ModuleSystem *ms, const char *path);

/* Resolve an import AST node. Handles single, path, and multi-import forms.
   Returns number of modules loaded, or -1 on error. */
int module_resolve_import(ModuleSystem *ms, AstNode *import_node);

/* Add an external .o path from a link directive */
void module_add_link_obj(ModuleSystem *ms, const char *path);

/* Parse a .hdef file into a list of declarations (type defs + fn declarations).
   Returns a NODE_PROGRAM node or NULL on error. */
AstNode *module_parse_hdef(ModuleSystem *ms, const char *path);

#endif
