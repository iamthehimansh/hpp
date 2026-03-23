#ifndef HPP_BACKEND_H
#define HPP_BACKEND_H

#include <stdbool.h>

#define MAX_STDLIB_OBJS 8

typedef struct {
    const char *asm_file;
    const char *obj_file;
    const char *output_file;
    /* stdlib object files to link */
    const char *stdlib_objs[MAX_STDLIB_OBJS];
    int stdlib_obj_count;
    bool asm_only;
} BackendConfig;

bool backend_assemble(BackendConfig *cfg);
bool backend_link(BackendConfig *cfg);
bool backend_run(BackendConfig *cfg, const char *asm_source);

/* Assemble a single .asm file to .o (for building stdlib) */
bool backend_assemble_file(const char *asm_path, const char *obj_path);

#endif
