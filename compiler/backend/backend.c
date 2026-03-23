#include "backend/backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool backend_assemble_file(const char *asm_path, const char *obj_path) {
    char cmd[1024];
    int n = snprintf(cmd, sizeof(cmd), "nasm -f elf64 -o %s %s",
                     obj_path, asm_path);
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "error: assemble command too long\n");
        return false;
    }
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "error: assembler failed for '%s' (exit code %d)\n",
                asm_path, ret);
        return false;
    }
    return true;
}

bool backend_assemble(BackendConfig *cfg) {
    return backend_assemble_file(cfg->asm_file, cfg->obj_file);
}

bool backend_link(BackendConfig *cfg) {
    char cmd[4096];
    int n = snprintf(cmd, sizeof(cmd), "ld -o %s %s",
                     cfg->output_file, cfg->obj_file);
    /* Append each stdlib object file */
    for (int i = 0; i < cfg->stdlib_obj_count; i++) {
        size_t cur = (size_t)n;
        int m = snprintf(cmd + cur, sizeof(cmd) - cur, " %s",
                         cfg->stdlib_objs[i]);
        if (m < 0 || (size_t)m >= sizeof(cmd) - cur) {
            fprintf(stderr, "error: link command too long\n");
            return false;
        }
        n += m;
    }
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "error: link command too long\n");
        return false;
    }
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "error: linker failed (exit code %d)\n", ret);
        return false;
    }
    return true;
}

bool backend_run(BackendConfig *cfg, const char *asm_source) {
    /* Write assembly source to file */
    FILE *f = fopen(cfg->asm_file, "w");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s' for writing\n",
                cfg->asm_file);
        return false;
    }
    size_t len = strlen(asm_source);
    size_t written = fwrite(asm_source, 1, len, f);
    fclose(f);
    if (written != len) {
        fprintf(stderr, "error: failed to write assembly to '%s'\n",
                cfg->asm_file);
        return false;
    }

    if (cfg->asm_only) {
        return true;
    }

    /* Assemble user code */
    if (!backend_assemble(cfg)) {
        return false;
    }

    /* Link everything */
    if (!backend_link(cfg)) {
        return false;
    }

    /* Clean up intermediate files */
    remove(cfg->asm_file);
    remove(cfg->obj_file);

    return true;
}
