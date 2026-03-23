// backend.c rewritten in H++
// Invokes nasm and ld via fork+execve instead of libc system()

import std/{sys, mem, str, io, printf};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn arena_alloc(arena: long, size: long) -> long;
fn str_len(s: long) -> long;
fn str_eq(a: long, b: long) -> int;

// BackendConfig layout (must match C):
// asm_file@0, obj_file@8, output_file@16,
// stdlib_objs@24 (8 pointers * 8 bytes = 64), stdlib_obj_count@88, asm_only@92
const BC_ASM_FILE  = 0;
const BC_OBJ_FILE  = 8;
const BC_OUT_FILE  = 16;
const BC_STDLIB    = 24;
const BC_STDLIB_CNT = 88;
const BC_ASM_ONLY  = 92;

// Run a shell command via fork+execve("/bin/sh", ["/bin/sh", "-c", cmd])
fn run_command(cmd: long) -> int {
    int pid = sys_fork();
    if (pid == 0) {
        // Child process
        // Build argv: ["/bin/sh", "-c", cmd, NULL]
        long argv = sys_mmap(0, 4096, 3, 34, -1, 0);
        long sh = "/bin/sh";
        long dash_c = "-c";
        mem_write64(argv, 0, sh);
        mem_write64(argv, 8, dash_c);
        mem_write64(argv, 16, cmd);
        mem_write64(argv, 24, 0);
        // envp = NULL
        sys_execve(sh, argv, 0);
        sys_exit(127);
    }
    // Parent — wait for child
    long status_buf = sys_mmap(0, 4096, 3, 34, -1, 0);
    sys_wait4(pid, status_buf, 0, 0);
    int status = mem_read32(status_buf, 0);
    sys_munmap(status_buf, 4096);
    // Extract exit code: WEXITSTATUS = (status >> 8) & 0xFF
    int exit_code = (status >> 8) & 255;
    return exit_code;
}

// Build a command string in a buffer: concatenate parts with spaces
fn cmd_start(buf: long, s: long) -> int {
    long len = str_len(s);
    mem_copy(buf, s, len);
    return int(len);
}

fn cmd_append(buf: long, p: int, s: long) -> int {
    long slen = str_len(s);
    int pos = p;
    mem_write8(buf, pos, 32);  // space
    pos++;
    long off = pos;
    long dst = buf + off;
    mem_copy(dst, s, slen);
    return pos + int(slen);
}

fn backend_assemble_file(asm_path: long, obj_path: long) -> int {
    // Build: "nasm -f elf64 -o OBJ ASM"
    long cmd = sys_mmap(0, 4096, 3, 34, -1, 0);
    int p = cmd_start(cmd, "nasm -f elf64 -o");
    p = cmd_append(cmd, p, obj_path);
    p = cmd_append(cmd, p, asm_path);
    mem_write8(cmd, p, 0);  // null terminate

    int ret = run_command(cmd);
    sys_munmap(cmd, 4096);

    if (ret != 0) {
        fmt_err("error: assembler failed for '%s' (exit code %d)\n", asm_path, ret, 0, 0);
        return 0;
    }
    return 1;
}

fn backend_assemble(cfg: long) -> int {
    long asm_file = mem_read64(cfg, BC_ASM_FILE);
    long obj_file = mem_read64(cfg, BC_OBJ_FILE);
    return backend_assemble_file(asm_file, obj_file);
}

fn backend_link(cfg: long) -> int {
    long cmd = sys_mmap(0, 4096, 3, 34, -1, 0);
    long out_file = mem_read64(cfg, BC_OUT_FILE);
    long obj_file = mem_read64(cfg, BC_OBJ_FILE);

    // Build: "ld -o OUTPUT OBJ [STDLIB_OBJS...]"
    int p = cmd_start(cmd, "ld -o");
    p = cmd_append(cmd, p, out_file);
    p = cmd_append(cmd, p, obj_file);

    // Append stdlib objects
    int stdlib_cnt = mem_read32(cfg, BC_STDLIB_CNT);
    for (int i = 0; i < stdlib_cnt; i++) {
        long obj = mem_read64(cfg, BC_STDLIB + i * 8);
        if (obj != 0) {
            p = cmd_append(cmd, p, obj);
        }
    }
    mem_write8(cmd, p, 0);

    int ret = run_command(cmd);
    sys_munmap(cmd, 4096);

    if (ret != 0) {
        fmt_err("error: linker failed (exit code %d)\n", ret, 0, 0, 0);
        return 0;
    }
    return 1;
}

fn backend_run(cfg: long, asm_source: long) -> int {
    long asm_file = mem_read64(cfg, BC_ASM_FILE);

    // Write asm to file
    int fd = sys_open(asm_file, 577, 420);  // O_WRONLY|O_CREAT|O_TRUNC=577, mode=0644=420
    if (fd < 0) {
        fmt_err("error: cannot open '%s' for writing\n", asm_file, 0, 0, 0);
        return 0;
    }
    long len = str_len(asm_source);
    long written = sys_write(fd, asm_source, len);
    sys_close(fd);
    if (written != len) {
        fmt_err("error: failed to write assembly to '%s'\n", asm_file, 0, 0, 0);
        return 0;
    }

    // If asm_only, stop here
    int asm_only = mem_read8(cfg, BC_ASM_ONLY);
    if (asm_only != 0) {
        return 1;
    }

    // Assemble
    if (backend_assemble(cfg) == 0) {
        return 0;
    }

    // Link
    if (backend_link(cfg) == 0) {
        return 0;
    }

    // Clean up intermediate files
    sys_unlink(asm_file);
    long obj_file = mem_read64(cfg, BC_OBJ_FILE);
    sys_unlink(obj_file);

    return 1;
}
