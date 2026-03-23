// module.c rewritten in H++
// Module resolution: find .hdef/.asm/.o/.hpp files, parse .hdef

import std/{sys, mem, str, io, printf};

def int  bit[32];
def long bit[64];
def byte bit[8];

fn arena_alloc(arena: long, size: long) -> long;
fn arena_strdup(arena: long, s: long) -> long;
fn arena_strndup(arena: long, s: long, len: long) -> long;
fn str_len(s: long) -> long;
fn str_eq(a: long, b: long) -> int;
fn mem_cmp(a: long, b: long, n: long) -> int;
fn file_read_all(arena: long, path: long, out_len: long) -> long;
fn lexer_init(lex: long, source: long, source_len: long, filename: long, arena: long, strtab: long);
fn lexer_lex_all(lex: long, out: long) -> int;
fn parser_init(p: long, tokens: long, count: int, arena: long, strtab: long, filename: long);
fn parser_parse(p: long) -> long;

// Module (64 bytes)
const MOD_NAME    = 0;
const MOD_HDEF    = 8;
const MOD_ASM     = 16;
const MOD_HPP     = 24;
const MOD_PREOBJ  = 32;
const MOD_OBJ     = 40;
const MOD_DECLS   = 48;
const MOD_LOADED  = 56;
const MOD_SIZE    = 64;

// ModuleSystem (4712 bytes)
const MS_ARENA    = 0;
const MS_STRTAB   = 8;
const MS_PATHS    = 16;     // 8 * 8 = 64 bytes
const MS_PATHCNT  = 80;
const MS_MODULES  = 88;     // 64 * 64 = 4096 bytes
const MS_MODCNT   = 4184;
const MS_LINKS    = 4192;   // 64 * 8 = 512 bytes
const MS_LINKCNT  = 4704;
const MS_SIZE     = 4712;

const MAX_MODULES = 64;
const MAX_SEARCH_PATHS = 8;

// AstNode import_decl offsets (from AN_AS = 32)
// segments@0(8B), segment_count@8(4B), names@16(8B), name_count@24(4B)
const IMP_SEGS    = 32;
const IMP_SEGCNT  = 40;
const IMP_NAMES   = 48;
const IMP_NAMECNT = 56;

fn path_join_mod(arena: long, a: long, b: long) -> long {
    long al = str_len(a);
    long bl = str_len(b);
    long out = arena_alloc(arena, al + 1 + bl + 1);
    mem_copy(out, a, al);
    mem_write8(out, int(al), 47); // '/'
    long dst = out + al + 1;
    mem_copy(dst, b, bl);
    mem_write8(out, int(al + 1 + bl), 0);
    return out;
}

fn file_exists_mod(path: long) -> int {
    int r = sys_access(path, 0);
    if (r == 0) { return 1; }
    return 0;
}

fn segments_to_path(arena: long, segs: long, count: int) -> long {
    long total = 0;
    for (int i = 0; i < count; i++) {
        if (i > 0) { total++; }
        long seg = mem_read64(segs, i * 8);
        total += str_len(seg);
    }
    long out = arena_alloc(arena, total + 1);
    long pos = 0;
    for (int i = 0; i < count; i++) {
        if (i > 0) { mem_write8(out, int(pos), 47); pos++; }
        long seg = mem_read64(segs, i * 8);
        long sl = str_len(seg);
        long d = out + pos;
        mem_copy(d, seg, sl);
        pos += sl;
    }
    mem_write8(out, int(pos), 0);
    return out;
}

fn module_system_create(arena: long, strtab: long) -> long {
    long ms = arena_alloc(arena, MS_SIZE);
    mem_write64(ms, MS_ARENA, arena);
    mem_write64(ms, MS_STRTAB, strtab);
    mem_write32(ms, MS_PATHCNT, 0);
    mem_write32(ms, MS_MODCNT, 0);
    mem_write32(ms, MS_LINKCNT, 0);
    return ms;
}

fn module_add_search_path(ms: long, path: long) {
    int cnt = mem_read32(ms, MS_PATHCNT);
    if (cnt >= MAX_SEARCH_PATHS) { return; }
    long arena = mem_read64(ms, MS_ARENA);
    long dup = arena_strdup(arena, path);
    mem_write64(ms, MS_PATHS + cnt * 8, dup);
    mem_write32(ms, MS_PATHCNT, cnt + 1);
}

fn module_add_link_obj(ms: long, path: long) {
    int cnt = mem_read32(ms, MS_LINKCNT);
    if (cnt >= MAX_MODULES) { return; }
    long arena = mem_read64(ms, MS_ARENA);
    long dup = arena_strdup(arena, path);
    mem_write64(ms, MS_LINKS + cnt * 8, dup);
    mem_write32(ms, MS_LINKCNT, cnt + 1);
}

fn find_loaded(ms: long, name: long) -> long {
    int cnt = mem_read32(ms, MS_MODCNT);
    for (int i = 0; i < cnt; i++) {
        long i_off = i * MOD_SIZE;
        long mod = ms + MS_MODULES + i_off;
        long mname = mem_read64(mod, MOD_NAME);
        if (str_eq(mname, name) != 0) { return mod; }
    }
    return 0;
}

fn module_parse_hdef(ms: long, path: long) -> long {
    long arena = mem_read64(ms, MS_ARENA);
    long strtab = mem_read64(ms, MS_STRTAB);
    long out_len_buf = arena_alloc(arena, 8);
    long source = file_read_all(arena, path, out_len_buf);
    if (source == 0) { return 0; }
    long source_len = mem_read64(out_len_buf, 0);

    // Lexer struct is 136 bytes
    long lex = arena_alloc(arena, 136);
    lexer_init(lex, source, source_len, path, arena, strtab);
    long tokens_ptr = arena_alloc(arena, 8);
    int token_count = lexer_lex_all(lex, tokens_ptr);
    long tokens = mem_read64(tokens_ptr, 0);
    if (token_count <= 0) { return 0; }

    // Parser struct — need to know size
    // Let's allocate generously
    long parser = arena_alloc(arena, 2104);  // exact Parser size
    parser_init(parser, tokens, token_count, arena, strtab, path);
    long program = parser_parse(parser);
    return program;
}

fn make_obj_path(arena: long, mod_path: long) -> long {
    // Build /tmp/hpp_mod_PATH.o with / replaced by _
    long prefix = "/tmp/hpp_mod_";
    long plen = str_len(prefix);
    long mlen = str_len(mod_path);
    long out = arena_alloc(arena, plen + mlen + 3);
    mem_copy(out, prefix, plen);
    long pos = plen;
    for (long i = 0; i < mlen; i++) {
        int ch = mem_read8(mod_path, int(i));
        if (ch == 47) { ch = 95; } // '/' → '_'
        mem_write8(out, int(pos), ch);
        pos++;
    }
    mem_write8(out, int(pos), 46); pos++; // '.'
    mem_write8(out, int(pos), 111); pos++; // 'o'
    mem_write8(out, int(pos), 0);
    return out;
}

fn module_resolve(ms: long, mod_path: long) -> long {
    long existing = find_loaded(ms, mod_path);
    if (existing != 0) { return existing; }

    int cnt = mem_read32(ms, MS_MODCNT);
    if (cnt >= MAX_MODULES) { return 0; }

    long arena = mem_read64(ms, MS_ARENA);

    // Build relative paths
    long hdef_rel = arena_alloc(arena, str_len(mod_path) + 10);
    mem_copy(hdef_rel, mod_path, str_len(mod_path));
    long hlen = str_len(mod_path);
    long suf_hdef = ".hdef";
    mem_copy(hdef_rel + hlen, suf_hdef, 6);

    long asm_rel = arena_alloc(arena, hlen + 10);
    mem_copy(asm_rel, mod_path, hlen);
    long suf_asm = ".asm";
    mem_copy(asm_rel + hlen, suf_asm, 5);

    long hpp_rel = arena_alloc(arena, hlen + 10);
    mem_copy(hpp_rel, mod_path, hlen);
    long suf_hpp = ".hpp";
    mem_copy(hpp_rel + hlen, suf_hpp, 5);

    long obj_rel = arena_alloc(arena, hlen + 10);
    mem_copy(obj_rel, mod_path, hlen);
    long suf_obj = ".o";
    mem_copy(obj_rel + hlen, suf_obj, 3);

    long found_hdef = 0;
    long found_asm = 0;
    long found_hpp = 0;
    long found_obj = 0;

    int pcnt = mem_read32(ms, MS_PATHCNT);
    for (int i = 0; i < pcnt; i++) {
        long base = mem_read64(ms, MS_PATHS + i * 8);
        if (found_hdef == 0) {
            long try = path_join_mod(arena, base, hdef_rel);
            if (file_exists_mod(try) != 0) { found_hdef = try; }
        }
        if (found_asm == 0) {
            long try_a = path_join_mod(arena, base, asm_rel);
            if (file_exists_mod(try_a) != 0) { found_asm = try_a; }
        }
        if (found_hpp == 0) {
            long try_h = path_join_mod(arena, base, hpp_rel);
            if (file_exists_mod(try_h) != 0) { found_hpp = try_h; }
        }
        if (found_obj == 0) {
            long try_o = path_join_mod(arena, base, obj_rel);
            if (file_exists_mod(try_o) != 0) { found_obj = try_o; }
        }
    }

    if (found_hdef == 0) {
        if (found_hpp == 0) {
            fmt_err("error: cannot find module '%s'\n", mod_path, 0, 0, 0);
            return 0;
        }
    }

    // Parse .hdef or .hpp
    long decls = 0;
    if (found_hdef != 0) {
        decls = module_parse_hdef(ms, found_hdef);
    } else {
        if (found_hpp != 0) {
            decls = module_parse_hdef(ms, found_hpp);
        }
    }

    long cnt_off = cnt * MOD_SIZE;
    long mod = ms + MS_MODULES + cnt_off;
    mem_zero(mod, MOD_SIZE);
    mem_write64(mod, MOD_NAME, arena_strdup(arena, mod_path));
    mem_write64(mod, MOD_HDEF, found_hdef);
    mem_write64(mod, MOD_ASM, found_asm);
    mem_write64(mod, MOD_HPP, found_hpp);
    mem_write64(mod, MOD_PREOBJ, found_obj);
    mem_write64(mod, MOD_DECLS, decls);
    mem_write8(mod, MOD_LOADED, 1);
    mem_write64(mod, MOD_OBJ, make_obj_path(arena, mod_path));
    mem_write32(ms, MS_MODCNT, cnt + 1);

    return mod;
}

// List submodules by scanning directory for .hdef files using getdents64
fn list_submodules(ms: long, dir_path: long, prefix: long, out: long, max: int) -> int {
    int count = 0;
    long arena = mem_read64(ms, MS_ARENA);
    int pcnt = mem_read32(ms, MS_PATHCNT);

    for (int pi = 0; pi < pcnt; pi++) {
        if (count >= max) { break; }
        long base = mem_read64(ms, MS_PATHS + pi * 8);
        long full = path_join_mod(arena, base, dir_path);

        // O_RDONLY | O_DIRECTORY = 0x10000 = 65536
        int fd = sys_open(full, 65536, 0);
        if (fd < 0) { continue; }

        // Read directory entries
        long buf = sys_mmap(0, 8192, 3, 34, -1, 0);
        if (buf == 0) { sys_close(fd); continue; }
        long nread_l = sys_getdents64(fd, buf, 8192);
        sys_close(fd);
        int nread = int(nread_l);

        if (nread <= 0) { sys_munmap(buf, 8192); continue; }

        // Parse dirent64 entries
        int bpos = 0;
        while (bpos < nread) {
            if (count >= max) { break; }
            // dirent64: d_ino@0(8B), d_off@8(8B), d_reclen@16(2B), d_type@18(1B), d_name@19
            int reclen = mem_read16(buf, bpos + 16);
            if (reclen == 0) { break; }  // safety: avoid infinite loop
            long bp = bpos;
            long d_name = buf + bp + 19;
            long nlen = str_len(d_name);

            // Check if ends with ".hdef"
            if (nlen > 5) {
                long suffix = d_name + nlen - 5;
                if (str_eq(suffix, ".hdef") != 0) {
                    // Strip .hdef
                    long name = arena_strndup(arena, d_name, nlen - 5);
                    long full_name = name;
                    if (prefix != 0) {
                        long plen = str_len(prefix);
                        if (plen > 0) {
                            full_name = path_join_mod(arena, prefix, name);
                        }
                    }
                    mem_write64(out, count * 8, full_name);
                    count++;
                }
            }
            bpos += reclen;
        }
        sys_munmap(buf, 8192);
    }
    return count;
}

fn mem_read16(ptr: long, offset: int) -> int {
    // Read 16-bit little-endian
    int lo = mem_read8(ptr, offset);
    int hi = mem_read8(ptr, offset + 1);
    return lo + hi * 256;
}

fn module_resolve_import(ms: long, import_node: long) -> int {
    // import_node is an AstNode with kind NODE_IMPORT
    long segs = mem_read64(import_node, IMP_SEGS);
    int seg_count = mem_read32(import_node, IMP_SEGCNT);
    long names = mem_read64(import_node, IMP_NAMES);
    int name_count = mem_read32(import_node, IMP_NAMECNT);

    long arena = mem_read64(ms, MS_ARENA);
    long base_path = segments_to_path(arena, segs, seg_count);

    if (name_count > 0) {
        // Multi-import: import std/{io, mem}
        int loaded = 0;
        for (int i = 0; i < name_count; i++) {
            long n = mem_read64(names, i * 8);
            long full = path_join_mod(arena, base_path, n);
            long m = module_resolve(ms, full);
            if (m != 0) { loaded++; }
        }
        return loaded;
    }

    // Try as directory first (import std → scan all .hdef)
    long sub_out = arena_alloc(arena, MAX_MODULES * 8);
    int sub_count = list_submodules(ms, base_path, base_path, sub_out, MAX_MODULES);
    if (sub_count > 0) {
        int loaded = 0;
        for (int i = 0; i < sub_count; i++) {
            long sname = mem_read64(sub_out, i * 8);
            long m = module_resolve(ms, sname);
            if (m != 0) { loaded++; }
        }
        return loaded;
    }

    // Try as single module
    long m = module_resolve(ms, base_path);
    if (m != 0) { return 1; }

    return 0 - 1;
}
