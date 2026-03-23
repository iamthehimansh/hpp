#define _GNU_SOURCE
#include "common/args.h"
#include "common/arena.h"
#include "common/error.h"
#include "common/file_io.h"
#include "common/strtab.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "ast/ast.h"
#include "parser/parser.h"
#include "sema/sema.h"
#include "codegen/codegen.h"
#include "backend/backend.h"
#include "common/module.h"
#include "common/macro.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

/* Find the directory containing the compiler binary */
static const char *get_compiler_dir(Arena *arena, const char *argv0) {
    char buf[4096];
    /* Try /proc/self/exe first (Linux) */
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
    } else {
        /* Fallback: use argv[0] */
        size_t alen = strlen(argv0);
        if (alen >= sizeof(buf)) alen = sizeof(buf) - 1;
        memcpy(buf, argv0, alen);
        buf[alen] = '\0';
    }
    /* Get directory part */
    char *dir = dirname(buf);
    return arena_strdup(arena, dir);
}

static char *path_join(Arena *arena, const char *dir, const char *file) {
    size_t dlen = strlen(dir);
    size_t flen = strlen(file);
    char *out = arena_alloc(arena, dlen + 1 + flen + 1);
    memcpy(out, dir, dlen);
    out[dlen] = '/';
    memcpy(out + dlen + 1, file, flen);
    out[dlen + 1 + flen] = '\0';
    return out;
}

static bool find_loaded_module(ModuleSystem *ms, const char *name) {
    for (int i = 0; i < ms->module_count; i++) {
        if (strcmp(ms->modules[i].name, name) == 0) return true;
    }
    return false;
}

static char *derive_output_name(Arena *arena, const char *input, const char *ext) {
    size_t len = strlen(input);
    // Find last dot
    const char *dot = NULL;
    for (size_t i = len; i > 0; i--) {
        if (input[i - 1] == '.') { dot = input + i - 1; break; }
        if (input[i - 1] == '/' || input[i - 1] == '\\') break;
    }
    size_t base_len = dot ? (size_t)(dot - input) : len;
    size_t ext_len = ext ? strlen(ext) : 0;
    char *out = arena_alloc(arena, base_len + ext_len + 1);
    memcpy(out, input, base_len);
    if (ext) memcpy(out + base_len, ext, ext_len);
    out[base_len + ext_len] = '\0';
    return out;
}

int main(int argc, char **argv) {
    CompilerArgs cargs;
    if (!args_parse(argc, argv, &cargs)) {
        return 1;
    }

    error_init();
    Arena *arena = arena_create(0);
    StrTab *strtab = strtab_create(arena);

    // Read source file
    size_t source_len = 0;
    char *source = file_read_all(arena, cargs.input_file, &source_len);
    if (!source) {
        fprintf(stderr, "error: cannot open file '%s'\n", cargs.input_file);
        arena_destroy(arena);
        return 1;
    }

    // Lex
    Lexer lexer;
    lexer_init(&lexer, source, source_len, cargs.input_file, arena, strtab);
    Token *tokens = NULL;
    int token_count = lexer_lex_all(&lexer, &tokens);

    if (error_has_errors()) {
        arena_destroy(arena);
        return 1;
    }

    // Macro processing (before parsing)
    MacroProcessor *macros = macro_create(arena);
    int expanded_count = 0;
    Token *expanded = macro_process(macros, tokens, token_count, &expanded_count);
    if (expanded && expanded_count > 0) {
        tokens = expanded;
        token_count = expanded_count;
    }

    if (cargs.dump_tokens) {
        for (int i = 0; i < token_count; i++) {
            token_print(&tokens[i]);
        }
        arena_destroy(arena);
        return 0;
    }

    // Parse
    Parser parser;
    parser_init(&parser, tokens, token_count, arena, strtab, cargs.input_file);
    AstNode *program = parser_parse(&parser);

    if (error_has_errors() || !program) {
        arena_destroy(arena);
        return 1;
    }

    if (cargs.dump_ast) {
        ast_print(program, 0);
        arena_destroy(arena);
        return 0;
    }

    // Set up module system
    const char *compiler_dir = get_compiler_dir(arena, argv[0]);
    ModuleSystem *modsys = module_system_create(arena, strtab);

    // Add search paths: stdlib dir, then current directory
    module_add_search_path(modsys,
        path_join(arena, compiler_dir, "../compiler/stdlib"));
    module_add_search_path(modsys,
        path_join(arena, compiler_dir, "stdlib"));
    module_add_search_path(modsys, ".");

    // Semantic analysis
    Sema sema;
    sema_init(&sema, arena, strtab, cargs.input_file);
    if (cargs.lib_mode) sema.is_library = true;

    // Process imports and link directives BEFORE sema
    for (AstNodeList *n = program->as.program.decls; n; n = n->next) {
        if (!n->node) continue;
        if (n->node->kind == NODE_IMPORT) {
            int loaded = module_resolve_import(modsys, n->node);
            if (loaded < 0) {
                arena_destroy(arena);
                return 1;
            }
        } else if (n->node->kind == NODE_LINK) {
            module_add_link_obj(modsys, n->node->as.link_decl.path);
        }
    }

    // Register all imported module declarations in sema
    bool needs_std_libs = false;
    for (int i = 0; i < modsys->module_count; i++) {
        if (modsys->modules[i].declarations) {
            sema_register_module(&sema, modsys->modules[i].declarations);
        }
        // If any std/ module was imported, we need the monolithic .asm files
        if (strncmp(modsys->modules[i].name, "std/", 4) == 0 ||
            strcmp(modsys->modules[i].name, "std") == 0) {
            needs_std_libs = true;
        }
    }

    // Ensure monolithic std_ll.asm and std_util.asm are linked for std/ imports
    if (needs_std_libs) {
        const char *mono_libs[] = {"std_ll", "std_util"};
        for (int li = 0; li < 2; li++) {
            if (find_loaded_module(modsys, mono_libs[li])) continue;
            char asm_rel[64];
            snprintf(asm_rel, sizeof(asm_rel), "%s.asm", mono_libs[li]);
            for (int s = 0; s < modsys->search_path_count; s++) {
                char *try_path = path_join(arena, modsys->search_paths[s], asm_rel);
                if (access(try_path, F_OK) == 0) {
                    if (modsys->module_count < MAX_MODULES) {
                        Module *cm = &modsys->modules[modsys->module_count++];
                        memset(cm, 0, sizeof(Module));
                        cm->name = mono_libs[li];
                        cm->asm_path = try_path;
                        char obj_name[64];
                        snprintf(obj_name, sizeof(obj_name), "/tmp/hpp_mod_%s.o", mono_libs[li]);
                        cm->obj_path = arena_strdup(arena, obj_name);
                        cm->loaded = true;
                    }
                    break;
                }
            }
        }
    }

    if (!sema_analyze(&sema, program)) {
        arena_destroy(arena);
        return 1;
    }

    // Code generation
    CodeGen codegen;
    codegen_init(&codegen, arena, sema.types, sema.symbols, sema.opps, cargs.target);
    if (cargs.lib_mode) codegen.is_library = true;
    char *asm_output = codegen_generate(&codegen, program);

    if (error_has_errors() || !asm_output) {
        arena_destroy(arena);
        return 1;
    }

    // Backend
    const char *output_file = cargs.output_file;
    if (!output_file) {
        output_file = cargs.asm_only
            ? derive_output_name(arena, cargs.input_file, ".asm")
            : derive_output_name(arena, cargs.input_file, NULL);
    }

    const char *asm_file = cargs.asm_only
        ? output_file
        : derive_output_name(arena, cargs.input_file, ".asm");

    BackendConfig bcfg;
    memset(&bcfg, 0, sizeof(bcfg));
    bcfg.asm_file = asm_file;
    bcfg.obj_file = derive_output_name(arena, cargs.input_file, ".o");
    bcfg.output_file = output_file;
    bcfg.asm_only = cargs.asm_only;

    if (!cargs.asm_only) {
        // Build/link all imported modules
        for (int i = 0; i < modsys->module_count; i++) {
            Module *mod = &modsys->modules[i];
            const char *obj_to_link = NULL;

            if (mod->prebuilt_obj) {
                // Pre-compiled .o — link directly
                obj_to_link = mod->prebuilt_obj;
            } else if (mod->asm_path && access(mod->asm_path, F_OK) == 0) {
                // .asm source — assemble to .o
                if (!backend_assemble_file(mod->asm_path, mod->obj_path)) {
                    arena_destroy(arena);
                    return 1;
                }
                obj_to_link = mod->obj_path;
            } else if (mod->hpp_path && access(mod->hpp_path, F_OK) == 0) {
                // .hpp source — compile to .asm then assemble to .o
                // Read, lex, parse, sema, codegen the .hpp file
                size_t hpp_len = 0;
                char *hpp_src = file_read_all(arena, mod->hpp_path, &hpp_len);
                if (hpp_src) {
                    Lexer hpp_lex;
                    lexer_init(&hpp_lex, hpp_src, hpp_len, mod->hpp_path, arena, strtab);
                    Token *hpp_tokens = NULL;
                    int hpp_tc = lexer_lex_all(&hpp_lex, &hpp_tokens);

                    Parser hpp_parser;
                    parser_init(&hpp_parser, hpp_tokens, hpp_tc, arena, strtab, mod->hpp_path);
                    AstNode *hpp_prog = parser_parse(&hpp_parser);

                    if (hpp_prog && !error_has_errors()) {
                        // Register any imports in the .hpp file
                        for (AstNodeList *n = hpp_prog->as.program.decls; n; n = n->next) {
                            if (n->node && n->node->kind == NODE_IMPORT)
                                module_resolve_import(modsys, n->node);
                        }

                        Sema hpp_sema;
                        sema_init(&hpp_sema, arena, strtab, mod->hpp_path);
                        hpp_sema.is_library = true;  // no main() required
                        // Register imported modules for the .hpp file too
                        for (int j = 0; j < modsys->module_count; j++) {
                            if (modsys->modules[j].declarations)
                                sema_register_module(&hpp_sema, modsys->modules[j].declarations);
                        }

                        if (sema_analyze(&hpp_sema, hpp_prog)) {
                            CodeGen hpp_cg;
                            codegen_init(&hpp_cg, arena, hpp_sema.types,
                                         hpp_sema.symbols, hpp_sema.opps, cargs.target);
                            hpp_cg.is_library = true;  // no _start, export all
                            char *hpp_asm = codegen_generate(&hpp_cg, hpp_prog);
                            if (hpp_asm) {
                                // Write .asm and assemble
                                char tmp_asm[512];
                                snprintf(tmp_asm, sizeof(tmp_asm), "%s.asm", mod->obj_path);
                                FILE *f = fopen(tmp_asm, "w");
                                if (f) {
                                    fwrite(hpp_asm, 1, strlen(hpp_asm), f);
                                    fclose(f);
                                    backend_assemble_file(tmp_asm, mod->obj_path);
                                    remove(tmp_asm);
                                    obj_to_link = mod->obj_path;
                                }
                            }
                        }
                    }
                }
            }

            if (obj_to_link && bcfg.stdlib_obj_count < MAX_STDLIB_OBJS) {
                bcfg.stdlib_objs[bcfg.stdlib_obj_count++] = obj_to_link;
            }
        }
        // Add external link objects
        for (int i = 0; i < modsys->link_obj_count; i++) {
            if (bcfg.stdlib_obj_count < MAX_STDLIB_OBJS) {
                bcfg.stdlib_objs[bcfg.stdlib_obj_count++] = modsys->link_objs[i];
            }
        }
    }

    if (!backend_run(&bcfg, asm_output)) {
        arena_destroy(arena);
        return 1;
    }

    arena_destroy(arena);
    return 0;
}
