#define _GNU_SOURCE
#include "common/module.h"
#include "common/file_io.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static char *path_join(Arena *arena, const char *a, const char *b) {
    size_t al = strlen(a);
    size_t bl = strlen(b);
    char *out = arena_alloc(arena, al + 1 + bl + 1);
    memcpy(out, a, al);
    out[al] = '/';
    memcpy(out + al + 1, b, bl);
    out[al + 1 + bl] = '\0';
    return out;
}

static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

/* Build a module path string from import segments: ["std","io"] -> "std/io" */
static const char *segments_to_path(Arena *arena, const char **segs, int count) {
    size_t total = 0;
    for (int i = 0; i < count; i++) {
        if (i > 0) total++;  /* for '/' */
        total += strlen(segs[i]);
    }
    char *out = arena_alloc(arena, total + 1);
    size_t pos = 0;
    for (int i = 0; i < count; i++) {
        if (i > 0) out[pos++] = '/';
        size_t sl = strlen(segs[i]);
        memcpy(out + pos, segs[i], sl);
        pos += sl;
    }
    out[pos] = '\0';
    return out;
}

/* ------------------------------------------------------------------ */
/*  Module System                                                      */
/* ------------------------------------------------------------------ */

ModuleSystem *module_system_create(Arena *arena, StrTab *strtab) {
    ModuleSystem *ms = arena_alloc(arena, sizeof(ModuleSystem));
    ms->arena = arena;
    ms->strtab = strtab;
    ms->search_path_count = 0;
    ms->module_count = 0;
    ms->link_obj_count = 0;
    return ms;
}

void module_add_search_path(ModuleSystem *ms, const char *path) {
    if (ms->search_path_count < MAX_SEARCH_PATHS) {
        ms->search_paths[ms->search_path_count++] = arena_strdup(ms->arena, path);
    }
}

void module_add_link_obj(ModuleSystem *ms, const char *path) {
    if (ms->link_obj_count < MAX_MODULES) {
        ms->link_objs[ms->link_obj_count++] = arena_strdup(ms->arena, path);
    }
}

/* Check if module already loaded */
static Module *find_loaded(ModuleSystem *ms, const char *name) {
    for (int i = 0; i < ms->module_count; i++) {
        if (strcmp(ms->modules[i].name, name) == 0) {
            return &ms->modules[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Parse .hdef file                                                   */
/* ------------------------------------------------------------------ */

AstNode *module_parse_hdef(ModuleSystem *ms, const char *path) {
    size_t source_len = 0;
    char *source = file_read_all(ms->arena, path, &source_len);
    if (!source) {
        fprintf(stderr, "error: cannot read module file '%s'\n", path);
        return NULL;
    }

    Lexer lex;
    lexer_init(&lex, source, source_len, path, ms->arena, ms->strtab);
    Token *tokens = NULL;
    int token_count = lexer_lex_all(&lex, &tokens);
    if (token_count <= 0) return NULL;

    Parser parser;
    parser_init(&parser, tokens, token_count, ms->arena, ms->strtab, path);
    AstNode *program = parser_parse(&parser);
    return program;
}

/* ------------------------------------------------------------------ */
/*  Resolve a single module by path                                    */
/* ------------------------------------------------------------------ */

Module *module_resolve(ModuleSystem *ms, const char *mod_path) {
    /* Already loaded? */
    Module *existing = find_loaded(ms, mod_path);
    if (existing) return existing;

    if (ms->module_count >= MAX_MODULES) {
        fprintf(stderr, "error: too many modules loaded\n");
        return NULL;
    }

    /* Search for .hdef, .asm, .hpp, and .o files in search paths */
    char hdef_rel[512], asm_rel[512], hpp_rel[512], obj_rel[512];
    snprintf(hdef_rel, sizeof(hdef_rel), "%s.hdef", mod_path);
    snprintf(asm_rel, sizeof(asm_rel), "%s.asm", mod_path);
    snprintf(hpp_rel, sizeof(hpp_rel), "%s.hpp", mod_path);
    snprintf(obj_rel, sizeof(obj_rel), "%s.o", mod_path);

    const char *found_hdef = NULL;
    const char *found_asm = NULL;
    const char *found_hpp = NULL;
    const char *found_obj = NULL;

    for (int i = 0; i < ms->search_path_count; i++) {
        const char *base = ms->search_paths[i];
        char *try_hdef = path_join(ms->arena, base, hdef_rel);
        char *try_asm  = path_join(ms->arena, base, asm_rel);
        char *try_hpp  = path_join(ms->arena, base, hpp_rel);
        char *try_obj  = path_join(ms->arena, base, obj_rel);

        if (!found_hdef && file_exists(try_hdef)) found_hdef = try_hdef;
        if (!found_asm  && file_exists(try_asm))  found_asm  = try_asm;
        if (!found_hpp  && file_exists(try_hpp))  found_hpp  = try_hpp;
        if (!found_obj  && file_exists(try_obj))  found_obj  = try_obj;
    }

    if (!found_hdef && !found_hpp) {
        fprintf(stderr, "error: cannot find module '%s' (searched for %s)\n",
                mod_path, hdef_rel);
        return NULL;
    }

    /* Parse the .hdef for type declarations.
       If no .hdef, fall back to parsing .hpp for declarations. */
    AstNode *decls = NULL;
    if (found_hdef) {
        decls = module_parse_hdef(ms, found_hdef);
    } else if (found_hpp) {
        decls = module_parse_hdef(ms, found_hpp);
    }

    Module *mod = &ms->modules[ms->module_count++];
    memset(mod, 0, sizeof(Module));
    mod->name = arena_strdup(ms->arena, mod_path);
    mod->hdef_path = found_hdef;
    mod->asm_path = found_asm;
    mod->hpp_path = found_hpp;
    mod->prebuilt_obj = found_obj;
    mod->declarations = decls;
    mod->loaded = true;

    /* Generate output .o path in /tmp (for compiling .asm/.hpp) */
    char obj_name[512];
    snprintf(obj_name, sizeof(obj_name), "/tmp/hpp_mod_");
    size_t pos = strlen(obj_name);
    for (const char *c = mod_path; *c && pos < sizeof(obj_name) - 3; c++) {
        obj_name[pos++] = (*c == '/') ? '_' : *c;
    }
    obj_name[pos++] = '.';
    obj_name[pos++] = 'o';
    obj_name[pos] = '\0';
    mod->obj_path = arena_strdup(ms->arena, obj_name);

    return mod;
}

/* ------------------------------------------------------------------ */
/*  Resolve an import directive — handles all forms                    */
/* ------------------------------------------------------------------ */

/* List submodules in a directory (scan for .hdef files) */
static int list_submodules(ModuleSystem *ms, const char *dir_path,
                           const char *prefix, const char **out, int max) {
    int count = 0;
    for (int i = 0; i < ms->search_path_count && count < max; i++) {
        char *full = path_join(ms->arena, ms->search_paths[i], dir_path);
        DIR *d = opendir(full);
        if (!d) continue;
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL && count < max) {
            size_t nlen = strlen(ent->d_name);
            if (nlen > 5 && strcmp(ent->d_name + nlen - 5, ".hdef") == 0) {
                /* Strip .hdef extension */
                char *name = arena_strndup(ms->arena, ent->d_name, nlen - 5);
                char *full_name;
                if (prefix && prefix[0]) {
                    full_name = path_join(ms->arena, prefix, name);
                } else {
                    full_name = name;
                }
                /* Avoid duplicates */
                bool dup = false;
                for (int j = 0; j < count; j++) {
                    if (strcmp(out[j], full_name) == 0) { dup = true; break; }
                }
                if (!dup) out[count++] = full_name;
            }
        }
        closedir(d);
    }
    return count;
}

int module_resolve_import(ModuleSystem *ms, AstNode *import_node) {
    if (import_node->kind != NODE_IMPORT) return -1;

    const char **segs = import_node->as.import_decl.segments;
    int seg_count = import_node->as.import_decl.segment_count;
    const char **names = import_node->as.import_decl.names;
    int name_count = import_node->as.import_decl.name_count;

    const char *base_path = segments_to_path(ms->arena, segs, seg_count);

    if (name_count > 0) {
        /* Multi-import: import std/{io, mem, str} */
        int loaded = 0;
        for (int i = 0; i < name_count; i++) {
            const char *full = path_join(ms->arena, base_path, names[i]);
            Module *m = module_resolve(ms, full);
            if (m) loaded++;
        }
        return loaded;
    }

    /* First try as a directory: import std → load all submodules in std/ */
    const char *submodules[MAX_MODULES];
    int sub_count = list_submodules(ms, base_path, base_path, submodules, MAX_MODULES);
    if (sub_count > 0) {
        int loaded = 0;
        for (int i = 0; i < sub_count; i++) {
            Module *sm = module_resolve(ms, submodules[i]);
            if (sm) loaded++;
        }
        return loaded;
    }

    /* Try as a single module: import std/io */
    Module *m = module_resolve(ms, base_path);
    if (m) return 1;

    return -1;
}
