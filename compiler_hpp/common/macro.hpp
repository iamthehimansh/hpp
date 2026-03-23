// macro.c rewritten in H++
// Token-level macro preprocessor: extract definitions, expand macros,
// rewrite arrays/brackets/structs/enums/sizeof/offsetof/compound-assign/inc-dec.

import std/{sys, mem, str, fmt};

def int  bit[32];
def long bit[64];
def byte bit[8];

// ---------------------------------------------------------------------------
//  External functions
// ---------------------------------------------------------------------------
fn arena_alloc(arena: long, size: long) -> long;
fn arena_strdup(arena: long, s: long) -> long;
fn arena_strndup(arena: long, s: long, len: long) -> long;
fn str_len(s: long) -> long;
fn str_eq(a: long, b: long) -> int;
fn mem_cmp(a: long, b: long, n: long) -> int;
fn itoa(n: int, buf: long) -> long;

// ---------------------------------------------------------------------------
//  Token kind constants (matching token.h enum)
// ---------------------------------------------------------------------------
const TOK_INT_LIT        = 0;
const TOK_BOOL_LIT       = 1;
const TOK_STRING_LIT     = 2;
const TOK_FN             = 3;
const TOK_RETURN         = 4;
const TOK_DEF            = 5;
const TOK_IF             = 6;
const TOK_ELSE           = 7;
const TOK_FOR            = 8;
const TOK_WHILE          = 9;
const TOK_BREAK          = 10;
const TOK_CONTINUE       = 11;
const TOK_CONST          = 12;
const TOK_LET            = 13;
const TOK_ASM            = 14;
const TOK_OPP            = 15;
const TOK_SWITCH         = 16;
const TOK_CASE           = 17;
const TOK_DEFAULT        = 18;
const TOK_TRUE           = 19;
const TOK_FALSE          = 20;
const TOK_NULL           = 21;
const TOK_BIT            = 22;
const TOK_IMPORT         = 23;
const TOK_LINK           = 24;
const TOK_MACRO          = 25;
const TOK_DEFX           = 26;
const TOK_STRUCT         = 27;
const TOK_ENUM           = 28;
const TOK_MATCH          = 29;
const TOK_AS             = 30;
const TOK_IDENT          = 31;
const TOK_PLUS           = 32;
const TOK_MINUS          = 33;
const TOK_STAR           = 34;
const TOK_SLASH          = 35;
const TOK_PERCENT        = 36;
const TOK_ASSIGN         = 37;
const TOK_EQUAL          = 38;
const TOK_NOT_EQUAL      = 39;
const TOK_LESS           = 40;
const TOK_GREATER        = 41;
const TOK_LESS_EQ        = 42;
const TOK_GREATER_EQ     = 43;
const TOK_AMP            = 44;
const TOK_PIPE           = 45;
const TOK_CARET          = 46;
const TOK_TILDE          = 47;
const TOK_BANG           = 48;
const TOK_SHL            = 49;
const TOK_SHR            = 50;
const TOK_AND            = 51;
const TOK_OR             = 52;
const TOK_PLUS_ASSIGN    = 53;
const TOK_MINUS_ASSIGN   = 54;
const TOK_STAR_ASSIGN    = 55;
const TOK_SLASH_ASSIGN   = 56;
const TOK_PERCENT_ASSIGN = 57;
const TOK_AMP_ASSIGN     = 58;
const TOK_PIPE_ASSIGN    = 59;
const TOK_CARET_ASSIGN   = 60;
const TOK_SHL_ASSIGN     = 61;
const TOK_SHR_ASSIGN     = 62;
const TOK_PLUS_PLUS      = 63;
const TOK_MINUS_MINUS    = 64;
const TOK_LPAREN         = 65;
const TOK_RPAREN         = 66;
const TOK_LBRACE         = 67;
const TOK_RBRACE         = 68;
const TOK_LBRACKET       = 69;
const TOK_RBRACKET       = 70;
const TOK_SEMICOLON      = 71;
const TOK_COMMA          = 72;
const TOK_COLON          = 73;
const TOK_DOT            = 74;
const TOK_ARROW          = 75;
const TOK_ASM_BODY       = 76;
const TOK_EOF            = 77;
const TOK_ERROR          = 78;

const ERR_PARSER = 1;

// ---------------------------------------------------------------------------
//  Token layout: 56 bytes each
//  kind@0(4B), pad@4(4B), loc.filename@8(8B), loc.line@16(4B), loc.col@20(4B),
//  text@24(8B), text_len@32(8B), int_value@40(8B), bool_value@48(1B)
// ---------------------------------------------------------------------------
const TK_KIND     = 0;
const TK_LOCFILE  = 8;
const TK_LOCLINE  = 16;
const TK_LOCCOL   = 20;
const TK_TEXT     = 24;
const TK_TEXT_LEN = 32;
const TK_INT_VAL  = 40;
const TK_BOOL_VAL = 48;
const TOK_SIZE    = 56;

// ---------------------------------------------------------------------------
//  MacroProcessor internal layout — H++-native, using long arrays
//  We store everything as flat long arrays for simplicity.
//
//  MP layout (all pointers, stored as longs):
//    arena@0           (8B)
//    macro_count@8     (4B)
//    macro_names@16    (long[256])   — 2048B  name ptrs
//    macro_paramcnts@2064 (int[256]) — 1024B
//    macro_params@3088 (long[256*16]) — 32768B  param name ptrs (flat)
//    macro_bodycnts@35856 (int[256])  — 1024B
//    macro_bodies@36880 (long[256*1024]) — 2097152B  body token ptrs (flat, each is ptr to 56B copy)
//
//  That's too big. Instead, use arena-allocated arrays of pointers.
//  Store: arena + 7 count/capacity pairs + 7 data pointers = ~120 bytes
// ---------------------------------------------------------------------------
//
//  Simpler approach: MacroProcessor as a small header struct, with
//  dynamically-allocated arrays via arena. Each "table" is a long[] or int[].
//
//  MP header (200 bytes):
//    arena          @0   (8B)
//    macro_count    @8   (4B)
//    arr_var_count  @12  (4B)
//    tw_count       @16  (4B)  type_width_count
//    struct_count   @20  (4B)
//    sv_count       @24  (4B)  struct_var_count
//    enum_count     @28  (4B)
//    const_count    @32  (4B)
//    -- arrays (allocated once at create, generous sizes) --
//    macro_names    @40  (8B) -> long[256]
//    macro_pcnts    @48  (8B) -> int[256]
//    macro_params   @56  (8B) -> long[256*16]  (flat: macro i param j = [i*16+j])
//    macro_bcnts    @64  (8B) -> int[256]
//    macro_bodies   @72  (8B) -> long[256]  (each entry -> arena-alloc'd Token copy array)
//    av_names       @80  (8B) -> long[256]  array var names
//    av_types       @88  (8B) -> long[256]  array var type strings
//    tw_names       @96  (8B) -> long[256]  type width names
//    tw_widths      @104 (8B) -> int[256]   type bit widths
//    st_names       @112 (8B) -> long[64]   struct names
//    st_fcnts       @120 (8B) -> int[64]    struct field counts
//    st_tsizes      @128 (8B) -> int[64]    struct total sizes
//    st_fnames      @136 (8B) -> long[64*32] struct field names  (flat: struct i field j = [i*32+j])
//    st_ftypes      @144 (8B) -> long[64*32] struct field types
//    st_fbwidths    @152 (8B) -> int[64*32]  struct field bit widths
//    st_fboffs      @160 (8B) -> int[64*32]  struct field byte offsets
//    sv_varnames    @168 (8B) -> long[256]   struct var var_name
//    sv_stnames     @176 (8B) -> long[256]   struct var struct_name
//    en_names       @184 (8B) -> long[64]    enum names
//    en_mcnts       @192 (8B) -> int[64]     enum member counts
//    en_mnames      @200 (8B) -> long[64*512] enum member names (flat)
//    en_mvals       @208 (8B) -> int[64*512]  enum member values (flat)
//    co_names       @216 (8B) -> long[256]   constant names
//    co_vals        @224 (8B) -> long[256]   constant values (uint64)
//  MP_SIZE = 232
// ---------------------------------------------------------------------------

const MP_ARENA       = 0;
const MP_MACRO_CNT   = 8;
const MP_AV_CNT      = 12;
const MP_TW_CNT      = 16;
const MP_ST_CNT      = 20;
const MP_SV_CNT      = 24;
const MP_EN_CNT      = 28;
const MP_CO_CNT      = 32;
// pad to 40
const MP_M_NAMES     = 40;
const MP_M_PCNTS     = 48;
const MP_M_PARAMS    = 56;
const MP_M_BCNTS     = 64;
const MP_M_BODIES    = 72;
const MP_AV_NAMES    = 80;
const MP_AV_TYPES    = 88;
const MP_TW_NAMES    = 96;
const MP_TW_WIDTHS   = 104;
const MP_ST_NAMES    = 112;
const MP_ST_FCNTS    = 120;
const MP_ST_TSIZES   = 128;
const MP_ST_FNAMES   = 136;
const MP_ST_FTYPES   = 144;
const MP_ST_FBWIDTHS = 152;
const MP_ST_FBOFFS   = 160;
const MP_SV_VARNAMES = 168;
const MP_SV_STNAMES  = 176;
const MP_EN_NAMES    = 184;
const MP_EN_MCNTS    = 192;
const MP_EN_MNAMES   = 200;
const MP_EN_MVALS    = 208;
const MP_CO_NAMES    = 216;
const MP_CO_VALS     = 224;
const MP_SIZE        = 232;

const MAX_MACROS       = 256;
const MAX_MACRO_PARAMS = 16;
const MAX_MACRO_BODY   = 1024;
const MAX_ARRAY_VARS   = 256;
const MAX_TYPE_DEFS    = 128;
const MAX_STRUCT_DEFS  = 64;
const MAX_STRUCT_FIELDS = 32;
const MAX_ENUM_DEFS    = 64;
const MAX_ENUM_MEMBERS = 512;
const MAX_CONSTANTS    = 256;

// ---------------------------------------------------------------------------
//  Helpers: read/write token fields from a contiguous token array
// ---------------------------------------------------------------------------

fn tok_ptr(tokens: long, i: int) -> long {
    return tokens + long(i) * long(TOK_SIZE);
}

fn tok_kind(tokens: long, i: int) -> int {
    return mem_read32(tok_ptr(tokens, i), TK_KIND);
}

fn tok_text(tokens: long, i: int) -> long {
    return mem_read64(tok_ptr(tokens, i), TK_TEXT);
}

fn tok_text_len(tokens: long, i: int) -> long {
    return mem_read64(tok_ptr(tokens, i), TK_TEXT_LEN);
}

fn tok_int_val(tokens: long, i: int) -> long {
    return mem_read64(tok_ptr(tokens, i), TK_INT_VAL);
}

fn tok_locfile(tokens: long, i: int) -> long {
    return mem_read64(tok_ptr(tokens, i), TK_LOCFILE);
}

fn tok_locline(tokens: long, i: int) -> int {
    return mem_read32(tok_ptr(tokens, i), TK_LOCLINE);
}

fn tok_loccol(tokens: long, i: int) -> int {
    return mem_read32(tok_ptr(tokens, i), TK_LOCCOL);
}

// Copy full 56-byte token from src array index si to dst array index di
fn tok_copy(dst: long, di: int, src: long, si: int) {
    long d = tok_ptr(dst, di);
    long s = tok_ptr(src, si);
    mem_copy(d, s, long(TOK_SIZE));
}

// Copy a single token pointer's data to dst[di]
fn tok_copy_from(dst: long, di: int, src_ptr: long) {
    long d = tok_ptr(dst, di);
    mem_copy(d, src_ptr, long(TOK_SIZE));
}

// Write a synthesized token into out[pos]
fn emit_synth(out: long, pos: int, locfile: long, locline: int, loccol: int,
              kind: int, text: long, text_len: long) -> int {
    long t = tok_ptr(out, pos);
    mem_zero(t, long(TOK_SIZE));
    mem_write32(t, TK_KIND, kind);
    mem_write64(t, TK_LOCFILE, locfile);
    mem_write32(t, TK_LOCLINE, locline);
    mem_write32(t, TK_LOCCOL, loccol);
    mem_write64(t, TK_TEXT, text);
    mem_write64(t, TK_TEXT_LEN, text_len);
    return pos + 1;
}

// Write an integer literal token into out[pos]
fn emit_int_tok(out: long, pos: int, locfile: long, locline: int, loccol: int,
                val: long, arena: long) -> int {
    // Convert int to string using itoa
    long buf = arena_alloc(arena, 24);
    itoa(int(val), buf);
    long slen = str_len(buf);
    long text = arena_strndup(arena, buf, slen);

    long t = tok_ptr(out, pos);
    mem_zero(t, long(TOK_SIZE));
    mem_write32(t, TK_KIND, TOK_INT_LIT);
    mem_write64(t, TK_LOCFILE, locfile);
    mem_write32(t, TK_LOCLINE, locline);
    mem_write32(t, TK_LOCCOL, loccol);
    mem_write64(t, TK_TEXT, text);
    mem_write64(t, TK_TEXT_LEN, slen);
    mem_write64(t, TK_INT_VAL, val);
    return pos + 1;
}

// Overwrite the loc of out[pos] from a source token array (copy loc from tokens[si])
fn set_loc_from(out: long, pos: int, tokens: long, si: int) {
    long d = tok_ptr(out, pos);
    long s = tok_ptr(tokens, si);
    mem_write64(d, TK_LOCFILE, mem_read64(s, TK_LOCFILE));
    mem_write32(d, TK_LOCLINE, mem_read32(s, TK_LOCLINE));
    mem_write32(d, TK_LOCCOL, mem_read32(s, TK_LOCCOL));
}

// ---------------------------------------------------------------------------
//  MP field accessors — long arrays use long_get/long_set (8B elements)
//  and int arrays use int_get/int_set (4B elements via mem_read/write32)
// ---------------------------------------------------------------------------

fn mp_arena(mp: long) -> long {
    return mem_read64(mp, MP_ARENA);
}

// Read a long from a long-array field at index
fn la_get(mp: long, field_off: int, idx: int) -> long {
    long arr = mem_read64(mp, field_off);
    return mem_read64(arr, idx * 8);
}

fn la_set(mp: long, field_off: int, idx: int, val: long) {
    long arr = mem_read64(mp, field_off);
    mem_write64(arr, idx * 8, val);
}

// Read/write an int from an int-array field at index
fn ia_get(mp: long, field_off: int, idx: int) -> int {
    long arr = mem_read64(mp, field_off);
    return mem_read32(arr, idx * 4);
}

fn ia_set(mp: long, field_off: int, idx: int, val: int) {
    long arr = mem_read64(mp, field_off);
    mem_write32(arr, idx * 4, val);
}

// ---------------------------------------------------------------------------
//  String comparison helper (compare text ptr + len with a known C string)
// ---------------------------------------------------------------------------
fn text_eq(text: long, tlen: long, lit: long) -> int {
    long llen = str_len(lit);
    if (tlen != llen) { return 0; }
    if (mem_cmp(text, lit, tlen) == 0) { return 1; }
    return 0;
}

// ---------------------------------------------------------------------------
//  build_func_name: concatenate type + suffix, arena-allocated
// ---------------------------------------------------------------------------
fn build_func_name(arena: long, type_str: long, suffix: long) -> long {
    long tlen = str_len(type_str);
    long slen = str_len(suffix);
    long buf = arena_alloc(arena, tlen + slen + 1);
    mem_copy(buf, type_str, tlen);
    mem_copy(buf + tlen, suffix, slen);
    mem_write8(buf, int(tlen + slen), 0);
    return buf;
}

// ---------------------------------------------------------------------------
//  Struct helpers
// ---------------------------------------------------------------------------

fn find_struct(mp: long, name: long) -> int {
    // Returns struct index or -1
    int cnt = mem_read32(mp, MP_ST_CNT);
    int i = 0;
    while (i < cnt) {
        long sn = la_get(mp, MP_ST_NAMES, i);
        if (str_eq(sn, name) != 0) { return i; }
        i = i + 1;
    }
    return -1;
}

fn find_field(mp: long, si: int, fname: long) -> int {
    // Returns field index within struct si, or -1
    int fcnt = ia_get(mp, MP_ST_FCNTS, si);
    int j = 0;
    while (j < fcnt) {
        int flat = si * MAX_STRUCT_FIELDS + j;
        long fn_ = la_get(mp, MP_ST_FNAMES, flat);
        if (str_eq(fn_, fname) != 0) { return j; }
        j = j + 1;
    }
    return -1;
}

fn register_struct_var(mp: long, var_name: long, struct_name: long) {
    int cnt = mem_read32(mp, MP_SV_CNT);
    if (cnt >= MAX_ARRAY_VARS) { return; }
    la_set(mp, MP_SV_VARNAMES, cnt, var_name);
    la_set(mp, MP_SV_STNAMES, cnt, struct_name);
    mem_write32(mp, MP_SV_CNT, cnt + 1);
}

// Returns struct index for a variable, or -1
fn lookup_struct_var(mp: long, var_name: long) -> int {
    int cnt = mem_read32(mp, MP_SV_CNT);
    int i = cnt - 1;
    while (i >= 0) {
        long vn = la_get(mp, MP_SV_VARNAMES, i);
        if (str_eq(vn, var_name) != 0) {
            long sn = la_get(mp, MP_SV_STNAMES, i);
            return find_struct(mp, sn);
        }
        i = i - 1;
    }
    return -1;
}

fn type_byte_size(mp: long, type_name: long) -> int {
    if (text_eq(type_name, str_len(type_name), "byte") != 0) { return 1; }
    if (text_eq(type_name, str_len(type_name), "int") != 0) { return 4; }
    if (text_eq(type_name, str_len(type_name), "long") != 0) { return 8; }
    int w = get_type_width(mp, type_name);
    if (w > 0) { return (w + 7) / 8; }
    return 4;
}

fn rw_suffix(bytes: int) -> long {
    if (bytes == 1) { return "8"; }
    if (bytes == 2) { return "16"; }
    if (bytes == 4) { return "32"; }
    if (bytes == 8) { return "64"; }
    return "32";
}

fn register_type_width(mp: long, name: long, width: int) {
    int cnt = mem_read32(mp, MP_TW_CNT);
    if (cnt >= MAX_TYPE_DEFS) { return; }
    la_set(mp, MP_TW_NAMES, cnt, name);
    ia_set(mp, MP_TW_WIDTHS, cnt, width);
    mem_write32(mp, MP_TW_CNT, cnt + 1);
}

fn get_type_width(mp: long, name: long) -> int {
    int cnt = mem_read32(mp, MP_TW_CNT);
    int i = 0;
    while (i < cnt) {
        long tn = la_get(mp, MP_TW_NAMES, i);
        if (str_eq(tn, name) != 0) {
            return ia_get(mp, MP_TW_WIDTHS, i);
        }
        i = i + 1;
    }
    return 0;
}

fn map_to_array_backend(mp: long, type_name: long) -> long {
    if (text_eq(type_name, str_len(type_name), "byte") != 0) { return "byte"; }
    if (text_eq(type_name, str_len(type_name), "int") != 0) { return "int"; }
    if (text_eq(type_name, str_len(type_name), "long") != 0) { return "long"; }
    int w = get_type_width(mp, type_name);
    if (w > 0) {
        if (w <= 8) { return "byte"; }
        if (w <= 32) { return "int"; }
        return "long";
    }
    return "int";
}

fn register_array_var(mp: long, var_name: long, type_name: long) {
    int cnt = mem_read32(mp, MP_AV_CNT);
    if (cnt >= MAX_ARRAY_VARS) { return; }
    long backend = map_to_array_backend(mp, type_name);
    la_set(mp, MP_AV_NAMES, cnt, var_name);
    la_set(mp, MP_AV_TYPES, cnt, backend);
    mem_write32(mp, MP_AV_CNT, cnt + 1);
}

fn register_string_var(mp: long, var_name: long) {
    int cnt = mem_read32(mp, MP_AV_CNT);
    if (cnt >= MAX_ARRAY_VARS) { return; }
    la_set(mp, MP_AV_NAMES, cnt, var_name);
    la_set(mp, MP_AV_TYPES, cnt, "byte");
    mem_write32(mp, MP_AV_CNT, cnt + 1);
}

fn lookup_array_type(mp: long, var_name: long) -> long {
    int cnt = mem_read32(mp, MP_AV_CNT);
    int i = cnt - 1;
    while (i >= 0) {
        long vn = la_get(mp, MP_AV_NAMES, i);
        if (str_eq(vn, var_name) != 0) {
            return la_get(mp, MP_AV_TYPES, i);
        }
        i = i - 1;
    }
    return "int";
}

fn find_macro(mp: long, name: long) -> int {
    // Returns macro index or -1
    int cnt = mem_read32(mp, MP_MACRO_CNT);
    int i = 0;
    while (i < cnt) {
        long mn = la_get(mp, MP_M_NAMES, i);
        if (str_eq(mn, name) != 0) { return i; }
        i = i + 1;
    }
    return -1;
}

// ---------------------------------------------------------------------------
//  macro_create
// ---------------------------------------------------------------------------
fn macro_create(arena: long) -> long {
    long mp = arena_alloc(arena, long(MP_SIZE));
    mem_write64(mp, MP_ARENA, arena);
    mem_write32(mp, MP_MACRO_CNT, 0);
    mem_write32(mp, MP_AV_CNT, 0);
    mem_write32(mp, MP_TW_CNT, 0);
    mem_write32(mp, MP_ST_CNT, 0);
    mem_write32(mp, MP_SV_CNT, 0);
    mem_write32(mp, MP_EN_CNT, 0);
    mem_write32(mp, MP_CO_CNT, 0);

    // Allocate all internal arrays
    mem_write64(mp, MP_M_NAMES,  arena_alloc(arena, long(MAX_MACROS) * 8));
    mem_write64(mp, MP_M_PCNTS,  arena_alloc(arena, long(MAX_MACROS) * 4));
    mem_write64(mp, MP_M_PARAMS, arena_alloc(arena, long(MAX_MACROS) * long(MAX_MACRO_PARAMS) * 8));
    mem_write64(mp, MP_M_BCNTS,  arena_alloc(arena, long(MAX_MACROS) * 4));
    mem_write64(mp, MP_M_BODIES, arena_alloc(arena, long(MAX_MACROS) * 8));
    mem_write64(mp, MP_AV_NAMES, arena_alloc(arena, long(MAX_ARRAY_VARS) * 8));
    mem_write64(mp, MP_AV_TYPES, arena_alloc(arena, long(MAX_ARRAY_VARS) * 8));
    mem_write64(mp, MP_TW_NAMES, arena_alloc(arena, long(MAX_TYPE_DEFS) * 8));
    mem_write64(mp, MP_TW_WIDTHS, arena_alloc(arena, long(MAX_TYPE_DEFS) * 4));
    mem_write64(mp, MP_ST_NAMES, arena_alloc(arena, long(MAX_STRUCT_DEFS) * 8));
    mem_write64(mp, MP_ST_FCNTS, arena_alloc(arena, long(MAX_STRUCT_DEFS) * 4));
    mem_write64(mp, MP_ST_TSIZES, arena_alloc(arena, long(MAX_STRUCT_DEFS) * 4));
    mem_write64(mp, MP_ST_FNAMES, arena_alloc(arena, long(MAX_STRUCT_DEFS) * long(MAX_STRUCT_FIELDS) * 8));
    mem_write64(mp, MP_ST_FTYPES, arena_alloc(arena, long(MAX_STRUCT_DEFS) * long(MAX_STRUCT_FIELDS) * 8));
    mem_write64(mp, MP_ST_FBWIDTHS, arena_alloc(arena, long(MAX_STRUCT_DEFS) * long(MAX_STRUCT_FIELDS) * 4));
    mem_write64(mp, MP_ST_FBOFFS, arena_alloc(arena, long(MAX_STRUCT_DEFS) * long(MAX_STRUCT_FIELDS) * 4));
    mem_write64(mp, MP_SV_VARNAMES, arena_alloc(arena, long(MAX_ARRAY_VARS) * 8));
    mem_write64(mp, MP_SV_STNAMES, arena_alloc(arena, long(MAX_ARRAY_VARS) * 8));
    mem_write64(mp, MP_EN_NAMES, arena_alloc(arena, long(MAX_ENUM_DEFS) * 8));
    mem_write64(mp, MP_EN_MCNTS, arena_alloc(arena, long(MAX_ENUM_DEFS) * 4));
    mem_write64(mp, MP_EN_MNAMES, arena_alloc(arena, long(MAX_ENUM_DEFS) * long(MAX_ENUM_MEMBERS) * 8));
    mem_write64(mp, MP_EN_MVALS, arena_alloc(arena, long(MAX_ENUM_DEFS) * long(MAX_ENUM_MEMBERS) * 4));
    mem_write64(mp, MP_CO_NAMES, arena_alloc(arena, long(MAX_CONSTANTS) * 8));
    mem_write64(mp, MP_CO_VALS, arena_alloc(arena, long(MAX_CONSTANTS) * 8));

    return mp;
}

// ---------------------------------------------------------------------------
//  Pass 1: Extract macro, defx, enum, const definitions
// ---------------------------------------------------------------------------
fn extract_macros(mp: long, tokens: long, count: int, out_count: long) -> long {
    long arena = mp_arena(mp);
    long out = arena_alloc(arena, long(count) * long(TOK_SIZE));
    int op = 0;
    int brace_depth = 0;
    int i = 0;

    while (i < count) {
        int kind = tok_kind(tokens, i);

        // ---- macro NAME(params) { body } ----
        if (kind == TOK_MACRO) {
            i = i + 1;
            if (i >= count) { continue; }
            if (tok_kind(tokens, i) != TOK_IDENT) { i = i + 1; continue; }
            int mc = mem_read32(mp, MP_MACRO_CNT);
            if (mc >= MAX_MACROS) { i = i + 1; continue; }

            long mname = tok_text(tokens, i);
            la_set(mp, MP_M_NAMES, mc, mname);
            ia_set(mp, MP_M_PCNTS, mc, 0);
            ia_set(mp, MP_M_BCNTS, mc, 0);
            i = i + 1;

            // Parse parameters
            int pcnt = 0;
            if (i < count) {
                if (tok_kind(tokens, i) == TOK_LPAREN) {
                    i = i + 1;
                    while (i < count) {
                        if (tok_kind(tokens, i) == TOK_RPAREN) { break; }
                        if (tok_kind(tokens, i) == TOK_IDENT) {
                            if (pcnt < MAX_MACRO_PARAMS) {
                                int pidx = mc * MAX_MACRO_PARAMS + pcnt;
                                la_set(mp, MP_M_PARAMS, pidx, tok_text(tokens, i));
                                pcnt = pcnt + 1;
                            }
                        }
                        i = i + 1;
                        if (i < count) {
                            if (tok_kind(tokens, i) == TOK_COMMA) { i = i + 1; }
                        }
                    }
                    if (i < count) { i = i + 1; } // skip ')'
                }
            }
            ia_set(mp, MP_M_PCNTS, mc, pcnt);

            // Parse body: { tokens } with brace nesting
            int bcnt = 0;
            if (i < count) {
                if (tok_kind(tokens, i) == TOK_LBRACE) {
                    i = i + 1;
                    // Allocate body token array (MAX_MACRO_BODY tokens)
                    long body = arena_alloc(arena, long(MAX_MACRO_BODY) * long(TOK_SIZE));
                    la_set(mp, MP_M_BODIES, mc, body);
                    int depth = 1;
                    while (i < count) {
                        if (depth == 0) { break; }
                        if (tok_kind(tokens, i) == TOK_LBRACE) { depth = depth + 1; }
                        if (tok_kind(tokens, i) == TOK_RBRACE) {
                            depth = depth - 1;
                            if (depth == 0) { break; }
                        }
                        if (bcnt < MAX_MACRO_BODY) {
                            tok_copy(body, bcnt, tokens, i);
                            bcnt = bcnt + 1;
                        }
                        i = i + 1;
                    }
                    if (i < count) { i = i + 1; } // skip '}'
                }
            }
            ia_set(mp, MP_M_BCNTS, mc, bcnt);
            mem_write32(mp, MP_MACRO_CNT, mc + 1);
            continue;
        }

        // ---- defx NAME { type field; ... } ----
        if (kind == TOK_DEFX) {
            i = i + 1;
            if (i >= count) { continue; }
            if (tok_kind(tokens, i) != TOK_IDENT) { i = i + 1; continue; }
            int sc = mem_read32(mp, MP_ST_CNT);
            if (sc >= MAX_STRUCT_DEFS) { i = i + 1; continue; }

            long sname = tok_text(tokens, i);
            la_set(mp, MP_ST_NAMES, sc, sname);
            ia_set(mp, MP_ST_FCNTS, sc, 0);
            ia_set(mp, MP_ST_TSIZES, sc, 0);
            i = i + 1;

            if (i < count) {
                if (tok_kind(tokens, i) == TOK_LBRACE) {
                    i = i + 1;
                    int offset = 0;
                    int fcnt = 0;
                    while (i < count) {
                        if (tok_kind(tokens, i) == TOK_RBRACE) { break; }
                        // Expect: TYPE FIELD_NAME
                        if (tok_kind(tokens, i) == TOK_IDENT) {
                            if (i + 1 < count) {
                                if (tok_kind(tokens, i + 1) == TOK_IDENT) {
                                    long ftype = tok_text(tokens, i);
                                    long fname = tok_text(tokens, i + 1);
                                    int fsize = type_byte_size(mp, ftype);
                                    if (fcnt < MAX_STRUCT_FIELDS) {
                                        int flat = sc * MAX_STRUCT_FIELDS + fcnt;
                                        la_set(mp, MP_ST_FNAMES, flat, fname);
                                        la_set(mp, MP_ST_FTYPES, flat, ftype);
                                        ia_set(mp, MP_ST_FBWIDTHS, flat, fsize * 8);
                                        ia_set(mp, MP_ST_FBOFFS, flat, offset);
                                        fcnt = fcnt + 1;
                                        offset = offset + fsize;
                                    }
                                    i = i + 2;
                                } else {
                                    i = i + 1;
                                }
                            } else {
                                i = i + 1;
                            }
                        } else {
                            i = i + 1;
                        }
                        // Skip separator
                        if (i < count) {
                            if (tok_kind(tokens, i) == TOK_COMMA) { i = i + 1; }
                            else { if (tok_kind(tokens, i) == TOK_SEMICOLON) { i = i + 1; } }
                        }
                    }
                    ia_set(mp, MP_ST_FCNTS, sc, fcnt);
                    ia_set(mp, MP_ST_TSIZES, sc, offset);
                    if (i < count) { i = i + 1; } // skip '}'
                }
            }
            // Skip optional trailing semicolon
            if (i < count) {
                if (tok_kind(tokens, i) == TOK_SEMICOLON) { i = i + 1; }
            }
            mem_write32(mp, MP_ST_CNT, sc + 1);
            continue;
        }

        // ---- enum NAME { MEMBER, MEMBER = VAL, ... } ----
        if (kind == TOK_ENUM) {
            i = i + 1;
            if (i >= count) { continue; }
            if (tok_kind(tokens, i) != TOK_IDENT) { i = i + 1; continue; }
            int ec = mem_read32(mp, MP_EN_CNT);
            if (ec >= MAX_ENUM_DEFS) { i = i + 1; continue; }

            long ename = tok_text(tokens, i);
            la_set(mp, MP_EN_NAMES, ec, ename);
            ia_set(mp, MP_EN_MCNTS, ec, 0);
            i = i + 1;

            if (i < count) {
                if (tok_kind(tokens, i) == TOK_LBRACE) {
                    i = i + 1;
                    int next_val = 0;
                    int mcnt = 0;
                    while (i < count) {
                        if (tok_kind(tokens, i) == TOK_RBRACE) { break; }
                        if (tok_kind(tokens, i) == TOK_IDENT) {
                            long memname = tok_text(tokens, i);
                            i = i + 1;
                            if (i < count) {
                                if (tok_kind(tokens, i) == TOK_ASSIGN) {
                                    i = i + 1;
                                    if (i < count) {
                                        if (tok_kind(tokens, i) == TOK_INT_LIT) {
                                            next_val = int(tok_int_val(tokens, i));
                                            i = i + 1;
                                        }
                                    }
                                }
                            }
                            if (mcnt < MAX_ENUM_MEMBERS) {
                                int flat = ec * MAX_ENUM_MEMBERS + mcnt;
                                la_set(mp, MP_EN_MNAMES, flat, memname);
                                ia_set(mp, MP_EN_MVALS, flat, next_val);
                                mcnt = mcnt + 1;
                            }
                            next_val = next_val + 1;
                        }
                        if (i < count) {
                            if (tok_kind(tokens, i) == TOK_COMMA) { i = i + 1; }
                        }
                    }
                    ia_set(mp, MP_EN_MCNTS, ec, mcnt);
                    if (i < count) { i = i + 1; } // skip '}'
                }
            }
            if (i < count) {
                if (tok_kind(tokens, i) == TOK_SEMICOLON) { i = i + 1; }
            }
            mem_write32(mp, MP_EN_CNT, ec + 1);
            continue;
        }

        // ---- const NAME = INT_LIT ; (top-level only) ----
        if (kind == TOK_CONST) {
            if (brace_depth == 0) {
                if (i + 3 < count) {
                    if (tok_kind(tokens, i + 1) == TOK_IDENT) {
                        if (tok_kind(tokens, i + 2) == TOK_ASSIGN) {
                            if (tok_kind(tokens, i + 3) == TOK_INT_LIT) {
                                int cc = mem_read32(mp, MP_CO_CNT);
                                if (cc < MAX_CONSTANTS) {
                                    la_set(mp, MP_CO_NAMES, cc, tok_text(tokens, i + 1));
                                    la_set(mp, MP_CO_VALS, cc, tok_int_val(tokens, i + 3));
                                    mem_write32(mp, MP_CO_CNT, cc + 1);
                                }
                                i = i + 4;
                                if (i < count) {
                                    if (tok_kind(tokens, i) == TOK_SEMICOLON) { i = i + 1; }
                                }
                                continue;
                            }
                        }
                    }
                }
            }
        }

        // Default: copy token, track brace depth
        if (kind == TOK_LBRACE) { brace_depth = brace_depth + 1; }
        if (kind == TOK_RBRACE) {
            if (brace_depth > 0) { brace_depth = brace_depth - 1; }
        }
        tok_copy(out, op, tokens, i);
        op = op + 1;
        i = i + 1;
    }

    mem_write32(out_count, 0, op);
    return out;
}

// ---------------------------------------------------------------------------
//  Pass 2: Expand macro invocations
// ---------------------------------------------------------------------------

fn is_macro_call(mp: long, tokens: long, count: int, pos: int) -> int {
    if (pos >= count) { return 0; }
    if (tok_kind(tokens, pos) != TOK_IDENT) { return 0; }
    if (find_macro(mp, tok_text(tokens, pos)) < 0) { return 0; }
    if (pos + 1 >= count) { return 0; }
    if (tok_kind(tokens, pos + 1) != TOK_LPAREN) { return 0; }
    return 1;
}

// Parse macro call arguments. tokens[start] should be '('.
// args_buf is a flat token buffer for all args (MAX_MACRO_PARAMS * 256 tokens).
// arg_counts is an int[MAX_MACRO_PARAMS] array.
// Returns: position after ')'. Writes nargs to nargs_ptr.
fn parse_macro_args(tokens: long, count: int, start: int,
                    args_buf: long, arg_counts: long, nargs_ptr: long) -> int {
    int pos = start;
    if (pos >= count) { mem_write32(nargs_ptr, 0, 0); return pos; }
    if (tok_kind(tokens, pos) != TOK_LPAREN) { mem_write32(nargs_ptr, 0, 0); return pos; }
    pos = pos + 1; // skip '('

    if (pos < count) {
        if (tok_kind(tokens, pos) == TOK_RPAREN) {
            mem_write32(nargs_ptr, 0, 0);
            return pos + 1;
        }
    }

    int cur_arg = 0;
    mem_write32(arg_counts, cur_arg * 4, 0);
    int depth = 0;

    while (pos < count) {
        int k = tok_kind(tokens, pos);
        if (k == TOK_LPAREN) { depth = depth + 1; }
        else {
            if (k == TOK_RPAREN) {
                if (depth == 0) {
                    cur_arg = cur_arg + 1;
                    pos = pos + 1;
                    break;
                }
                depth = depth - 1;
            } else {
                if (k == TOK_COMMA) {
                    if (depth == 0) {
                        cur_arg = cur_arg + 1;
                        if (cur_arg < MAX_MACRO_PARAMS) {
                            mem_write32(arg_counts, cur_arg * 4, 0);
                        }
                        pos = pos + 1;
                        continue;
                    }
                }
            }
        }

        if (cur_arg < MAX_MACRO_PARAMS) {
            int ac = mem_read32(arg_counts, cur_arg * 4);
            if (ac < 256) {
                // Store token at args_buf[cur_arg * 256 + ac]
                int flat_idx = cur_arg * 256 + ac;
                tok_copy(args_buf, flat_idx, tokens, pos);
                mem_write32(arg_counts, cur_arg * 4, ac + 1);
            }
        }
        pos = pos + 1;
    }

    mem_write32(nargs_ptr, 0, cur_arg);
    return pos;
}

fn expand_macros(mp: long, tokens: long, count: int, out_count: long) -> long {
    long arena = mp_arena(mp);
    int cap = count * 4 + 1024;
    long out = arena_alloc(arena, long(cap) * long(TOK_SIZE));
    int op = 0;

    // Allocate scratch buffers for macro arg parsing
    long args_buf = arena_alloc(arena, long(MAX_MACRO_PARAMS) * 256 * long(TOK_SIZE));
    long arg_counts = arena_alloc(arena, long(MAX_MACRO_PARAMS) * 4);
    long nargs_ptr = arena_alloc(arena, 4);

    int i = 0;
    while (i < count) {
        if (is_macro_call(mp, tokens, count, i) != 0) {
            int midx = find_macro(mp, tok_text(tokens, i));
            long call_locfile = tok_locfile(tokens, i);
            int call_locline = tok_locline(tokens, i);
            int call_loccol = tok_loccol(tokens, i);
            i = i + 1; // skip macro name

            // Parse arguments
            i = parse_macro_args(tokens, count, i, args_buf, arg_counts, nargs_ptr);
            int nargs = mem_read32(nargs_ptr, 0);

            int pcnt = ia_get(mp, MP_M_PCNTS, midx);
            int bcnt = ia_get(mp, MP_M_BCNTS, midx);
            long body = la_get(mp, MP_M_BODIES, midx);

            // Substitute body tokens
            int b = 0;
            while (b < bcnt) {
                if (op >= cap - 256) { break; }
                int bkind = tok_kind(body, b);
                int substituted = 0;
                if (bkind == TOK_IDENT) {
                    long btext = tok_text(body, b);
                    int p = 0;
                    while (p < pcnt) {
                        int pidx = midx * MAX_MACRO_PARAMS + p;
                        long pname = la_get(mp, MP_M_PARAMS, pidx);
                        if (str_eq(btext, pname) != 0) {
                            // Replace with argument tokens
                            int ac = mem_read32(arg_counts, p * 4);
                            int a = 0;
                            while (a < ac) {
                                int flat_idx = p * 256 + a;
                                tok_copy(out, op, args_buf, flat_idx);
                                // Overwrite loc with call loc
                                long tp = tok_ptr(out, op);
                                mem_write64(tp, TK_LOCFILE, call_locfile);
                                mem_write32(tp, TK_LOCLINE, call_locline);
                                mem_write32(tp, TK_LOCCOL, call_loccol);
                                op = op + 1;
                                a = a + 1;
                            }
                            substituted = 1;
                            break;
                        }
                        p = p + 1;
                    }
                }
                if (substituted == 0) {
                    tok_copy(out, op, body, b);
                    long tp = tok_ptr(out, op);
                    mem_write64(tp, TK_LOCFILE, call_locfile);
                    mem_write32(tp, TK_LOCLINE, call_locline);
                    mem_write32(tp, TK_LOCCOL, call_loccol);
                    op = op + 1;
                }
                b = b + 1;
            }
        } else {
            tok_copy(out, op, tokens, i);
            op = op + 1;
            i = i + 1;
        }
    }

    mem_write32(out_count, 0, op);
    return out;
}

// ---------------------------------------------------------------------------
//  Array declaration detection
// ---------------------------------------------------------------------------
fn is_array_decl(tokens: long, count: int, pos: int) -> int {
    if (pos + 2 >= count) { return 0; }
    if (tok_kind(tokens, pos) != TOK_IDENT) { return 0; }
    if (tok_kind(tokens, pos + 1) != TOK_IDENT) { return 0; }
    if (tok_kind(tokens, pos + 2) != TOK_LBRACKET) { return 0; }
    if (pos > 0) {
        int prev = tok_kind(tokens, pos - 1);
        if (prev == TOK_ASSIGN)    { return 0; }
        if (prev == TOK_PLUS)      { return 0; }
        if (prev == TOK_MINUS)     { return 0; }
        if (prev == TOK_STAR)      { return 0; }
        if (prev == TOK_SLASH)     { return 0; }
        if (prev == TOK_COMMA)     { return 0; }
        if (prev == TOK_LPAREN)    { return 0; }
        if (prev == TOK_EQUAL)     { return 0; }
        if (prev == TOK_NOT_EQUAL) { return 0; }
        if (prev == TOK_LESS)      { return 0; }
        if (prev == TOK_GREATER)   { return 0; }
        if (prev == TOK_RETURN)    { return 0; }
    }
    return 1;
}

// ---------------------------------------------------------------------------
//  Rewrite array declarations
// ---------------------------------------------------------------------------
fn rewrite_array_decls(mp: long, tokens: long, count: int, out_count: long) -> long {
    long arena = mp_arena(mp);
    int cap = count * 6 + 1024;
    long out = arena_alloc(arena, long(cap) * long(TOK_SIZE));
    int op = 0;

    // Scratch for size tokens and init values
    long size_buf = arena_alloc(arena, 64 * long(TOK_SIZE));
    long init_buf = arena_alloc(arena, 256 * long(TOK_SIZE));

    int i = 0;
    while (i < count) {
        if (is_array_decl(tokens, count, i) == 0) {
            tok_copy(out, op, tokens, i);
            op = op + 1;
            i = i + 1;
            continue;
        }

        long locfile = tok_locfile(tokens, i);
        int locline = tok_locline(tokens, i);
        int loccol = tok_loccol(tokens, i);
        long type_name = tok_text(tokens, i);
        long var_name = tok_text(tokens, i + 1);
        long var_len = tok_text_len(tokens, i + 1);
        i = i + 2; // skip TYPE and NAME

        long backend_type = map_to_array_backend(mp, type_name);
        long new_fn = build_func_name(arena, backend_type, "_new");
        long new_fn_len = str_len(new_fn);

        // Now at '['
        i = i + 1; // skip '['

        // Collect size expression
        int size_len = 0;
        while (i < count) {
            if (tok_kind(tokens, i) == TOK_RBRACKET) { break; }
            if (size_len < 64) {
                tok_copy(size_buf, size_len, tokens, i);
                size_len = size_len + 1;
            }
            i = i + 1;
        }
        if (i < count) { i = i + 1; } // skip ']'

        // Check for initializer: = [ val1, val2, ... ]
        int init_count = 0;
        int has_init = 0;

        if (i < count) {
            if (tok_kind(tokens, i) == TOK_ASSIGN) {
                i = i + 1; // skip '='
                if (i < count) {
                    if (tok_kind(tokens, i) == TOK_LBRACKET) {
                        i = i + 1; // skip '['
                        has_init = 1;
                        int depth = 0;
                        while (i < count) {
                            if (tok_kind(tokens, i) == TOK_RBRACKET) {
                                if (depth == 0) { break; }
                                depth = depth - 1;
                            }
                            if (tok_kind(tokens, i) == TOK_LBRACKET) { depth = depth + 1; }
                            if (tok_kind(tokens, i) == TOK_COMMA) {
                                if (depth == 0) {
                                    i = i + 1;
                                    continue;
                                }
                            }
                            if (init_count < 256) {
                                tok_copy(init_buf, init_count, tokens, i);
                                init_count = init_count + 1;
                            }
                            i = i + 1;
                        }
                        if (i < count) { i = i + 1; } // skip ']'
                    }
                }
            }
        }
        if (i < count) {
            if (tok_kind(tokens, i) == TOK_SEMICOLON) { i = i + 1; }
        }

        // Register array variable
        register_array_var(mp, var_name, type_name);

        // Emit: long VAR = TYPE_new(SIZE);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, "long", 4);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, var_name, var_len);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_ASSIGN, "=", 1);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, new_fn, new_fn_len);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
        if (size_len == 0) {
            if (has_init != 0) {
                op = emit_int_tok(out, op, locfile, locline, loccol, long(init_count), arena);
            }
        } else {
            int sj = 0;
            while (sj < size_len) {
                tok_copy(out, op, size_buf, sj);
                op = op + 1;
                sj = sj + 1;
            }
        }
        op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
        op = emit_synth(out, op, locfile, locline, loccol, TOK_SEMICOLON, ";", 1);

        // Emit initializer: TYPE_set(VAR, 0, val0); ...
        if (has_init != 0) {
            long set_fn = build_func_name(arena, backend_type, "_set");
            long set_fn_len = str_len(set_fn);
            int j = 0;
            while (j < init_count) {
                if (op >= cap - 16) { break; }
                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, set_fn, set_fn_len);
                op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, var_name, var_len);
                op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                op = emit_int_tok(out, op, locfile, locline, loccol, long(j), arena);
                op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                tok_copy(out, op, init_buf, j);
                op = op + 1;
                op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
                op = emit_synth(out, op, locfile, locline, loccol, TOK_SEMICOLON, ";", 1);
                j = j + 1;
            }
        }
    }

    mem_write32(out_count, 0, op);
    return out;
}

// ---------------------------------------------------------------------------
//  Bracket access helpers
// ---------------------------------------------------------------------------

fn collect_bracket_expr(tokens: long, count: int, start: int,
                        out_buf: long, out_len_ptr: long) -> int {
    int pos = start;
    if (pos >= count) { mem_write32(out_len_ptr, 0, 0); return pos; }
    if (tok_kind(tokens, pos) != TOK_LBRACKET) { mem_write32(out_len_ptr, 0, 0); return pos; }
    pos = pos + 1; // skip '['
    int depth = 1;
    int olen = 0;
    while (pos < count) {
        if (depth == 0) { break; }
        if (tok_kind(tokens, pos) == TOK_LBRACKET) { depth = depth + 1; }
        if (tok_kind(tokens, pos) == TOK_RBRACKET) {
            depth = depth - 1;
            if (depth == 0) { pos = pos + 1; break; }
        }
        if (olen < 256) {
            tok_copy(out_buf, olen, tokens, pos);
            olen = olen + 1;
        }
        pos = pos + 1;
    }
    mem_write32(out_len_ptr, 0, olen);
    return pos;
}

fn is_array_access(tokens: long, count: int, pos: int) -> int {
    if (pos >= count) { return 0; }
    if (tok_kind(tokens, pos) != TOK_IDENT) { return 0; }
    if (pos + 1 >= count) { return 0; }
    if (tok_kind(tokens, pos + 1) != TOK_LBRACKET) { return 0; }
    if (pos > 0) {
        int prev = tok_kind(tokens, pos - 1);
        if (prev == TOK_DEF) { return 0; }
        if (prev == TOK_COLON) { return 0; }
        if (prev == TOK_ARROW) { return 0; }
    }
    return 1;
}

// ---------------------------------------------------------------------------
//  Rewrite bracket syntax
// ---------------------------------------------------------------------------
fn rewrite_brackets(mp: long, tokens: long, count: int, out_count: long) -> long {
    long arena = mp_arena(mp);
    int cap = count * 4 + 256;
    long out = arena_alloc(arena, long(cap) * long(TOK_SIZE));
    int op = 0;

    long idx_buf = arena_alloc(arena, 256 * long(TOK_SIZE));
    long val_buf = arena_alloc(arena, 256 * long(TOK_SIZE));
    long idx_len_ptr = arena_alloc(arena, 4);

    int i = 0;
    while (i < count) {
        if (is_array_access(tokens, count, i) != 0) {
            long locfile = tok_locfile(tokens, i);
            int locline = tok_locline(tokens, i);
            int loccol = tok_loccol(tokens, i);

            // Save name token
            long name_text = tok_text(tokens, i);
            long name_tlen = tok_text_len(tokens, i);
            int name_idx = i;
            i = i + 1; // skip IDENT

            // Look up element type
            long etype = lookup_array_type(mp, name_text);
            long get_fn = build_func_name(arena, etype, "_get");
            long set_fn = build_func_name(arena, etype, "_set");
            long get_fn_len = str_len(get_fn);
            long set_fn_len = str_len(set_fn);

            // Collect index expression
            i = collect_bracket_expr(tokens, count, i, idx_buf, idx_len_ptr);
            int idx_len = mem_read32(idx_len_ptr, 0);

            // Check if followed by '='
            if (i < count) {
                if (tok_kind(tokens, i) == TOK_ASSIGN) {
                    i = i + 1; // skip '='

                    // Collect value expression until ; ) , }
                    int val_len = 0;
                    int depth = 0;
                    while (i < count) {
                        if (val_len >= 256) { break; }
                        int k = tok_kind(tokens, i);
                        if (k == TOK_LPAREN) { depth = depth + 1; }
                        else {
                            if (k == TOK_RPAREN) {
                                if (depth == 0) { break; }
                                depth = depth - 1;
                            } else {
                                if (depth == 0) {
                                    if (k == TOK_SEMICOLON) { break; }
                                    if (k == TOK_COMMA) { break; }
                                    if (k == TOK_RBRACE) { break; }
                                }
                            }
                        }
                        tok_copy(val_buf, val_len, tokens, i);
                        val_len = val_len + 1;
                        i = i + 1;
                    }

                    // Emit: TYPE_set(name, idx, val)
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, set_fn, set_fn_len);
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, name_text, name_tlen);
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                    int ij = 0;
                    while (ij < idx_len) {
                        if (op >= cap - 16) { break; }
                        tok_copy(out, op, idx_buf, ij);
                        op = op + 1;
                        ij = ij + 1;
                    }
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                    int vj = 0;
                    while (vj < val_len) {
                        if (op >= cap - 8) { break; }
                        tok_copy(out, op, val_buf, vj);
                        op = op + 1;
                        vj = vj + 1;
                    }
                    op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
                    continue;
                }
            }

            // Emit: TYPE_get(name, idx)
            op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, get_fn, get_fn_len);
            op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
            op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, name_text, name_tlen);
            op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
            int ij = 0;
            while (ij < idx_len) {
                if (op >= cap - 8) { break; }
                tok_copy(out, op, idx_buf, ij);
                op = op + 1;
                ij = ij + 1;
            }
            op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
        } else {
            tok_copy(out, op, tokens, i);
            op = op + 1;
            i = i + 1;
        }
    }

    mem_write32(out_count, 0, op);
    return out;
}

// ---------------------------------------------------------------------------
//  Struct dot-notation rewrite (single pass)
// ---------------------------------------------------------------------------
fn rewrite_dots(mp: long, tokens: long, count: int, out_count: long) -> long {
    long arena = mp_arena(mp);
    int cap = count * 6 + 1024;
    long out = arena_alloc(arena, long(cap) * long(TOK_SIZE));
    int op = 0;

    long val_buf = arena_alloc(arena, 256 * long(TOK_SIZE));

    int i = 0;
    while (i < count) {
        // Detect: StructName varname (declaration)
        int sd_idx = -1;
        if (i + 1 < count) {
            if (tok_kind(tokens, i) == TOK_IDENT) {
                if (tok_kind(tokens, i + 1) == TOK_IDENT) {
                    sd_idx = find_struct(mp, tok_text(tokens, i));
                    if (sd_idx >= 0) {
                        long locfile = tok_locfile(tokens, i);
                        int locline = tok_locline(tokens, i);
                        int loccol = tok_loccol(tokens, i);
                        long var_name = tok_text(tokens, i + 1);
                        long var_len = tok_text_len(tokens, i + 1);
                        long sname = la_get(mp, MP_ST_NAMES, sd_idx);
                        register_struct_var(mp, var_name, sname);

                        // Check: StructName var ;  (auto alloc)
                        if (i + 2 < count) {
                            if (tok_kind(tokens, i + 2) == TOK_SEMICOLON) {
                                int total_size = ia_get(mp, MP_ST_TSIZES, sd_idx);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, "long", 4);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, var_name, var_len);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_ASSIGN, "=", 1);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, "alloc", 5);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
                                op = emit_int_tok(out, op, locfile, locline, loccol, long(total_size), arena);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_SEMICOLON, ";", 1);
                                i = i + 3;
                                continue;
                            }
                        }

                        // StructName var = expr ; -> long var = expr ;
                        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, "long", 4);
                        i = i + 1; // skip struct type, keep var name
                        continue;
                    }
                }
            }
        }

        // Detect dot notation: IDENT . IDENT
        if (i + 2 < count) {
            if (tok_kind(tokens, i) == TOK_IDENT) {
                if (tok_kind(tokens, i + 1) == TOK_DOT) {
                    if (tok_kind(tokens, i + 2) == TOK_IDENT) {
                        int vsd = lookup_struct_var(mp, tok_text(tokens, i));
                        if (vsd >= 0) {
                            int fi = find_field(mp, vsd, tok_text(tokens, i + 2));
                            if (fi >= 0) {
                                long locfile = tok_locfile(tokens, i);
                                int locline = tok_locline(tokens, i);
                                int loccol = tok_loccol(tokens, i);
                                long var_text = tok_text(tokens, i);
                                long var_tlen = tok_text_len(tokens, i);
                                int flat = vsd * MAX_STRUCT_FIELDS + fi;
                                int foff = ia_get(mp, MP_ST_FBOFFS, flat);
                                int fbits = ia_get(mp, MP_ST_FBWIDTHS, flat);
                                int fbytes = fbits / 8;
                                long suf = rw_suffix(fbytes);
                                i = i + 3; // skip var . field

                                // Check if followed by '='
                                if (i < count) {
                                    if (tok_kind(tokens, i) == TOK_ASSIGN) {
                                        i = i + 1; // skip '='
                                        // Collect value
                                        int vl = 0;
                                        int dep = 0;
                                        while (i < count) {
                                            if (vl >= 256) { break; }
                                            int k = tok_kind(tokens, i);
                                            if (k == TOK_LPAREN) { dep = dep + 1; }
                                            else {
                                                if (k == TOK_RPAREN) {
                                                    if (dep == 0) { break; }
                                                    dep = dep - 1;
                                                } else {
                                                    if (dep == 0) {
                                                        if (k == TOK_SEMICOLON) { break; }
                                                        if (k == TOK_COMMA) { break; }
                                                        if (k == TOK_RBRACE) { break; }
                                                    }
                                                }
                                            }
                                            tok_copy(val_buf, vl, tokens, i);
                                            vl = vl + 1;
                                            i = i + 1;
                                        }
                                        // Emit: mem_writeN(var, offset, val)
                                        long wfn = build_func_name(arena, "mem_write", suf);
                                        long wfn_len = str_len(wfn);
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, wfn, wfn_len);
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, var_text, var_tlen);
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                                        op = emit_int_tok(out, op, locfile, locline, loccol, long(foff), arena);
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                                        int vj = 0;
                                        while (vj < vl) {
                                            tok_copy(out, op, val_buf, vj);
                                            op = op + 1;
                                            vj = vj + 1;
                                        }
                                        op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
                                        continue;
                                    }
                                }

                                // Emit: mem_readN(var, offset)
                                long rfn = build_func_name(arena, "mem_read", suf);
                                long rfn_len = str_len(rfn);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, rfn, rfn_len);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_LPAREN, "(", 1);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_IDENT, var_text, var_tlen);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_COMMA, ",", 1);
                                op = emit_int_tok(out, op, locfile, locline, loccol, long(foff), arena);
                                op = emit_synth(out, op, locfile, locline, loccol, TOK_RPAREN, ")", 1);
                                continue;
                            }
                        }
                    }
                }
            }
        }

        tok_copy(out, op, tokens, i);
        op = op + 1;
        i = i + 1;
    }

    mem_write32(out_count, 0, op);
    return out;
}

// ---------------------------------------------------------------------------
//  macro_process — main entry point
// ---------------------------------------------------------------------------
fn macro_process(mp: long, in_tokens: long, in_count: int, out_count: long) -> long {
    // TOP BYPASS
    mem_write32(out_count, 0, in_count);
    return in_tokens;
    long arena = mp_arena(mp);
    long tokens = in_tokens;
    int count = in_count;

    // Scratch for pass outputs
    long cnt_ptr = arena_alloc(arena, 4);

    // ---- Pass 0a: scan for type definitions (def NAME bit[N]) ----
    int ti = 0;
    while (ti < count - 3) {
        if (tok_kind(tokens, ti) == TOK_DEF) {
            if (tok_kind(tokens, ti + 1) == TOK_IDENT) {
                if (tok_kind(tokens, ti + 2) == TOK_BIT) {
                    if (ti + 5 < count) {
                        if (tok_kind(tokens, ti + 3) == TOK_LBRACKET) {
                            if (tok_kind(tokens, ti + 4) == TOK_INT_LIT) {
                                if (tok_kind(tokens, ti + 5) == TOK_RBRACKET) {
                                    register_type_width(mp, tok_text(tokens, ti + 1),
                                                        int(tok_int_val(tokens, ti + 4)));
                                }
                            }
                        }
                    }
                }
            }
        }
        ti = ti + 1;
    }

    // ---- Pass 0a2: rewrite sizeof(TYPE) -> int literal ----
    {
        int cap = count + 256;
        long rw = arena_alloc(arena, long(cap) * long(TOK_SIZE));
        int rp = 0;
        int si = 0;
        while (si < count) {
            // sizeof(TYPE) — simple type
            if (si + 3 < count) {
                if (tok_kind(tokens, si) == TOK_IDENT) {
                    long stxt = tok_text(tokens, si);
                    long stlen = tok_text_len(tokens, si);
                    if (text_eq(stxt, stlen, "sizeof") != 0) {
                        if (tok_kind(tokens, si + 1) == TOK_LPAREN) {
                            int k2 = tok_kind(tokens, si + 2);
                            if (k2 == TOK_IDENT) {
                                if (tok_kind(tokens, si + 3) == TOK_RPAREN) {
                                    int bytes = type_byte_size(mp, tok_text(tokens, si + 2));
                                    long lf = tok_locfile(tokens, si);
                                    int ll = tok_locline(tokens, si);
                                    int lc = tok_loccol(tokens, si);
                                    rp = emit_int_tok(rw, rp, lf, ll, lc, long(bytes), arena);
                                    si = si + 4;
                                    continue;
                                }
                            }
                            if (k2 == TOK_BIT) {
                                if (tok_kind(tokens, si + 3) == TOK_RPAREN) {
                                    int bytes = type_byte_size(mp, tok_text(tokens, si + 2));
                                    long lf = tok_locfile(tokens, si);
                                    int ll = tok_locline(tokens, si);
                                    int lc = tok_loccol(tokens, si);
                                    rp = emit_int_tok(rw, rp, lf, ll, lc, long(bytes), arena);
                                    si = si + 4;
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            // sizeof(bit[N]) — bit array type
            if (si + 6 < count) {
                if (tok_kind(tokens, si) == TOK_IDENT) {
                    long stxt = tok_text(tokens, si);
                    long stlen = tok_text_len(tokens, si);
                    if (text_eq(stxt, stlen, "sizeof") != 0) {
                        if (tok_kind(tokens, si + 1) == TOK_LPAREN) {
                            if (tok_kind(tokens, si + 2) == TOK_BIT) {
                                if (tok_kind(tokens, si + 3) == TOK_LBRACKET) {
                                    if (tok_kind(tokens, si + 4) == TOK_INT_LIT) {
                                        if (tok_kind(tokens, si + 5) == TOK_RBRACKET) {
                                            if (tok_kind(tokens, si + 6) == TOK_RPAREN) {
                                                int bits = int(tok_int_val(tokens, si + 4));
                                                int bytes = (bits + 7) / 8;
                                                long lf = tok_locfile(tokens, si);
                                                int ll = tok_locline(tokens, si);
                                                int lc = tok_loccol(tokens, si);
                                                rp = emit_int_tok(rw, rp, lf, ll, lc, long(bytes), arena);
                                                si = si + 7;
                                                continue;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            tok_copy(rw, rp, tokens, si);
            rp = rp + 1;
            si = si + 1;
        }
        tokens = rw;
        count = rp;
    }

    // ---- Pass 0b: scan for string variable assignments -> register as byte arrays ----
    {
        int si = 0;
        while (si < count - 3) {
            // Pattern: IDENT IDENT = STRING_LIT
            if (tok_kind(tokens, si) == TOK_IDENT) {
                if (tok_kind(tokens, si + 1) == TOK_IDENT) {
                    if (tok_kind(tokens, si + 2) == TOK_ASSIGN) {
                        if (tok_kind(tokens, si + 3) == TOK_STRING_LIT) {
                            register_string_var(mp, tok_text(tokens, si + 1));
                        }
                    }
                }
            }
            // Pattern: LET/CONST IDENT = STRING_LIT
            if (tok_kind(tokens, si) == TOK_LET) {
                if (tok_kind(tokens, si + 1) == TOK_IDENT) {
                    if (tok_kind(tokens, si + 2) == TOK_ASSIGN) {
                        if (tok_kind(tokens, si + 3) == TOK_STRING_LIT) {
                            register_string_var(mp, tok_text(tokens, si + 1));
                        }
                    }
                }
            }
            if (tok_kind(tokens, si) == TOK_CONST) {
                if (tok_kind(tokens, si + 1) == TOK_IDENT) {
                    if (tok_kind(tokens, si + 2) == TOK_ASSIGN) {
                        if (tok_kind(tokens, si + 3) == TOK_STRING_LIT) {
                            register_string_var(mp, tok_text(tokens, si + 1));
                        }
                    }
                }
            }
            si = si + 1;
        }
    }

    // ---- Pass 0c: rewrite compound assignment and ++/-- ----
    {
        int cap = count * 3 + 256;
        long rw = arena_alloc(arena, long(cap) * long(TOK_SIZE));
        int rp = 0;
        int ci = 0;
        while (ci < count) {
            // Prefix ++x or --x
            if (ci + 1 < count) {
                int ck = tok_kind(tokens, ci);
                if (ck == TOK_PLUS_PLUS) {
                    if (tok_kind(tokens, ci + 1) == TOK_IDENT) {
                        long lf = tok_locfile(tokens, ci);
                        int ll = tok_locline(tokens, ci);
                        int lc = tok_loccol(tokens, ci);
                        // x = x + 1
                        long xtext = tok_text(tokens, ci + 1);
                        long xtlen = tok_text_len(tokens, ci + 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_ASSIGN, "=", 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_PLUS, "+", 1);
                        rp = emit_int_tok(rw, rp, lf, ll, lc, 1, arena);
                        ci = ci + 2;
                        continue;
                    }
                }
                if (ck == TOK_MINUS_MINUS) {
                    if (tok_kind(tokens, ci + 1) == TOK_IDENT) {
                        long lf = tok_locfile(tokens, ci);
                        int ll = tok_locline(tokens, ci);
                        int lc = tok_loccol(tokens, ci);
                        long xtext = tok_text(tokens, ci + 1);
                        long xtlen = tok_text_len(tokens, ci + 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_ASSIGN, "=", 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_MINUS, "-", 1);
                        rp = emit_int_tok(rw, rp, lf, ll, lc, 1, arena);
                        ci = ci + 2;
                        continue;
                    }
                }
            }

            // Postfix x++ or x--
            if (ci + 1 < count) {
                if (tok_kind(tokens, ci) == TOK_IDENT) {
                    int nk = tok_kind(tokens, ci + 1);
                    if (nk == TOK_PLUS_PLUS) {
                        long lf = tok_locfile(tokens, ci);
                        int ll = tok_locline(tokens, ci);
                        int lc = tok_loccol(tokens, ci);
                        long xtext = tok_text(tokens, ci);
                        long xtlen = tok_text_len(tokens, ci);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_ASSIGN, "=", 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_PLUS, "+", 1);
                        rp = emit_int_tok(rw, rp, lf, ll, lc, 1, arena);
                        ci = ci + 2;
                        continue;
                    }
                    if (nk == TOK_MINUS_MINUS) {
                        long lf = tok_locfile(tokens, ci);
                        int ll = tok_locline(tokens, ci);
                        int lc = tok_loccol(tokens, ci);
                        long xtext = tok_text(tokens, ci);
                        long xtlen = tok_text_len(tokens, ci);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_ASSIGN, "=", 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_MINUS, "-", 1);
                        rp = emit_int_tok(rw, rp, lf, ll, lc, 1, arena);
                        ci = ci + 2;
                        continue;
                    }
                }
            }

            // Compound assignment: x += y -> x = x + (y)
            if (ci + 1 < count) {
                if (tok_kind(tokens, ci) == TOK_IDENT) {
                    int ck = tok_kind(tokens, ci + 1);
                    int binop = -1;
                    long binop_str = 0;
                    long binop_len = 0;

                    if (ck == TOK_PLUS_ASSIGN)    { binop = TOK_PLUS;    binop_str = "+";  binop_len = 1; }
                    if (ck == TOK_MINUS_ASSIGN)   { binop = TOK_MINUS;   binop_str = "-";  binop_len = 1; }
                    if (ck == TOK_STAR_ASSIGN)    { binop = TOK_STAR;    binop_str = "*";  binop_len = 1; }
                    if (ck == TOK_SLASH_ASSIGN)   { binop = TOK_SLASH;   binop_str = "/";  binop_len = 1; }
                    if (ck == TOK_PERCENT_ASSIGN) { binop = TOK_PERCENT; binop_str = "%";  binop_len = 1; }
                    if (ck == TOK_AMP_ASSIGN)     { binop = TOK_AMP;     binop_str = "&";  binop_len = 1; }
                    if (ck == TOK_PIPE_ASSIGN)    { binop = TOK_PIPE;    binop_str = "|";  binop_len = 1; }
                    if (ck == TOK_CARET_ASSIGN)   { binop = TOK_CARET;   binop_str = "^";  binop_len = 1; }
                    if (ck == TOK_SHL_ASSIGN)     { binop = TOK_SHL;     binop_str = "<<"; binop_len = 2; }
                    if (ck == TOK_SHR_ASSIGN)     { binop = TOK_SHR;     binop_str = ">>"; binop_len = 2; }

                    if (binop >= 0) {
                        long lf = tok_locfile(tokens, ci);
                        int ll = tok_locline(tokens, ci);
                        int lc = tok_loccol(tokens, ci);
                        long xtext = tok_text(tokens, ci);
                        long xtlen = tok_text_len(tokens, ci);
                        ci = ci + 2; // skip name and compound op

                        // Emit: name = name OP (
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_ASSIGN, "=", 1);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_IDENT, xtext, xtlen);
                        rp = emit_synth(rw, rp, lf, ll, lc, binop, binop_str, binop_len);
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_LPAREN, "(", 1);

                        // Copy RHS until ; ) , }
                        int dep = 0;
                        while (ci < count) {
                            if (rp >= cap - 8) { break; }
                            int k = tok_kind(tokens, ci);
                            if (k == TOK_LPAREN) { dep = dep + 1; }
                            else {
                                if (k == TOK_RPAREN) {
                                    if (dep == 0) { break; }
                                    dep = dep - 1;
                                } else {
                                    if (dep == 0) {
                                        if (k == TOK_SEMICOLON) { break; }
                                        if (k == TOK_COMMA) { break; }
                                        if (k == TOK_RBRACE) { break; }
                                    }
                                }
                            }
                            tok_copy(rw, rp, tokens, ci);
                            rp = rp + 1;
                            ci = ci + 1;
                        }
                        rp = emit_synth(rw, rp, lf, ll, lc, TOK_RPAREN, ")", 1);
                        continue;
                    }
                }
            }

            tok_copy(rw, rp, tokens, ci);
            rp = rp + 1;
            ci = ci + 1;
        }
        tokens = rw;
        count = rp;
    }

    // ---- Pass 1: extract macro, defx, enum, const definitions ----
    mem_write32(cnt_ptr, 0, 0);
    long stripped = extract_macros(mp, tokens, count, cnt_ptr);
    int stripped_count = mem_read32(cnt_ptr, 0);

    // DEBUG: return after Pass 1 extraction
    mem_write32(out_count, 0, stripped_count);
    return stripped;

    // ---- Pass 1a: rewrite enum member names and const names -> int literals ----
    int en_cnt = mem_read32(mp, MP_EN_CNT);
    int co_cnt = mem_read32(mp, MP_CO_CNT);
    if (en_cnt > 0) {
        int cap = stripped_count + 256;
        long rw = arena_alloc(arena, long(cap) * long(TOK_SIZE));
        int rp = 0;
        int ri = 0;
        while (ri < stripped_count) {
            if (tok_kind(stripped, ri) == TOK_IDENT) {
                long txt = tok_text(stripped, ri);
                int found = 0;
                // Check enum members
                int e = 0;
                while (e < en_cnt) {
                    if (found != 0) { break; }
                    int emc = ia_get(mp, MP_EN_MCNTS, e);
                    int m = 0;
                    while (m < emc) {
                        int flat = e * MAX_ENUM_MEMBERS + m;
                        long mname = la_get(mp, MP_EN_MNAMES, flat);
                        if (str_eq(txt, mname) != 0) {
                            int mval = ia_get(mp, MP_EN_MVALS, flat);
                            long lf = tok_locfile(stripped, ri);
                            int ll = tok_locline(stripped, ri);
                            int lc = tok_loccol(stripped, ri);
                            rp = emit_int_tok(rw, rp, lf, ll, lc, long(mval), arena);
                            found = 1;
                            break;
                        }
                        m = m + 1;
                    }
                    e = e + 1;
                }
                if (found == 0) {
                    tok_copy(rw, rp, stripped, ri);
                    rp = rp + 1;
                }
            } else {
                tok_copy(rw, rp, stripped, ri);
                rp = rp + 1;
            }
            ri = ri + 1;
        }
        stripped = rw;
        stripped_count = rp;
    }

    if (co_cnt > 0) {
        int cap = stripped_count + 256;
        long rw = arena_alloc(arena, long(cap) * long(TOK_SIZE));
        int rp = 0;
        int ri = 0;
        while (ri < stripped_count) {
            if (tok_kind(stripped, ri) == TOK_IDENT) {
                long txt = tok_text(stripped, ri);
                int found = 0;
                int c = 0;
                while (c < co_cnt) {
                    long cname = la_get(mp, MP_CO_NAMES, c);
                    if (str_eq(txt, cname) != 0) {
                        long cval = la_get(mp, MP_CO_VALS, c);
                        long lf = tok_locfile(stripped, ri);
                        int ll = tok_locline(stripped, ri);
                        int lc = tok_loccol(stripped, ri);
                        rp = emit_int_tok(rw, rp, lf, ll, lc, cval, arena);
                        found = 1;
                        break;
                    }
                    c = c + 1;
                }
                if (found == 0) {
                    tok_copy(rw, rp, stripped, ri);
                    rp = rp + 1;
                }
            } else {
                tok_copy(rw, rp, stripped, ri);
                rp = rp + 1;
            }
            ri = ri + 1;
        }
        stripped = rw;
        stripped_count = rp;
    }

    // ---- Pass 1a2: rewrite offsetof(StructName, field) -> int literal ----
    int st_cnt = mem_read32(mp, MP_ST_CNT);
    if (st_cnt > 0) {
        int cap = stripped_count + 256;
        long rw = arena_alloc(arena, long(cap) * long(TOK_SIZE));
        int rp = 0;
        int oi = 0;
        while (oi < stripped_count) {
            if (oi + 5 < stripped_count) {
                if (tok_kind(stripped, oi) == TOK_IDENT) {
                    long otxt = tok_text(stripped, oi);
                    long otlen = tok_text_len(stripped, oi);
                    if (text_eq(otxt, otlen, "offsetof") != 0) {
                        if (tok_kind(stripped, oi + 1) == TOK_LPAREN) {
                            if (tok_kind(stripped, oi + 2) == TOK_IDENT) {
                                if (tok_kind(stripped, oi + 3) == TOK_COMMA) {
                                    if (tok_kind(stripped, oi + 4) == TOK_IDENT) {
                                        if (tok_kind(stripped, oi + 5) == TOK_RPAREN) {
                                            int sidx = find_struct(mp, tok_text(stripped, oi + 2));
                                            if (sidx >= 0) {
                                                int fidx = find_field(mp, sidx, tok_text(stripped, oi + 4));
                                                if (fidx >= 0) {
                                                    int flat = sidx * MAX_STRUCT_FIELDS + fidx;
                                                    int boff = ia_get(mp, MP_ST_FBOFFS, flat);
                                                    long lf = tok_locfile(stripped, oi);
                                                    int ll = tok_locline(stripped, oi);
                                                    int lc = tok_loccol(stripped, oi);
                                                    rp = emit_int_tok(rw, rp, lf, ll, lc, long(boff), arena);
                                                    oi = oi + 6;
                                                    continue;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            tok_copy(rw, rp, stripped, oi);
            rp = rp + 1;
            oi = oi + 1;
        }
        stripped = rw;
        stripped_count = rp;
    }

    // ---- Pass 1b: rewrite struct declarations and dot notation ----
    if (st_cnt > 0) {
        mem_write32(cnt_ptr, 0, 0);
        stripped = rewrite_dots(mp, stripped, stripped_count, cnt_ptr);
        stripped_count = mem_read32(cnt_ptr, 0);

        // Run dot rewrite again (up to 8 passes) for nested dots
        int pass = 0;
        while (pass < 8) {
            mem_write32(cnt_ptr, 0, 0);
            long st2 = rewrite_dots(mp, stripped, stripped_count, cnt_ptr);
            int new_cnt = mem_read32(cnt_ptr, 0);
            if (new_cnt == stripped_count) {
                stripped = st2;
                stripped_count = new_cnt;
                break;
            }
            stripped = st2;
            stripped_count = new_cnt;
            pass = pass + 1;
        }
    }

    // ---- Pass 2: rewrite array declarations ----
    mem_write32(cnt_ptr, 0, 0);
    long after_decls = rewrite_array_decls(mp, stripped, stripped_count, cnt_ptr);
    int decl_count = mem_read32(cnt_ptr, 0);

    // ---- Pass 3: rewrite bracket syntax (multiple passes for nesting) ----
    long rewritten = after_decls;
    int rewritten_count = decl_count;
    {
        int pass = 0;
        while (pass < 8) {
            mem_write32(cnt_ptr, 0, 0);
            long rw = rewrite_brackets(mp, rewritten, rewritten_count, cnt_ptr);
            int new_cnt = mem_read32(cnt_ptr, 0);
            if (new_cnt == rewritten_count) {
                rewritten = rw;
                rewritten_count = new_cnt;
                break;
            }
            rewritten = rw;
            rewritten_count = new_cnt;
            pass = pass + 1;
        }
    }

    // ---- Pass 4: expand macros (up to 8 passes for nested macros) ----
    long current = rewritten;
    int current_count = rewritten_count;
    {
        int pass = 0;
        while (pass < 8) {
            mem_write32(cnt_ptr, 0, 0);
            long expanded = expand_macros(mp, current, current_count, cnt_ptr);
            int new_cnt = mem_read32(cnt_ptr, 0);
            if (new_cnt == current_count) {
                current = expanded;
                current_count = new_cnt;
                break;
            }
            current = expanded;
            current_count = new_cnt;
            pass = pass + 1;
        }
    }

    mem_write32(out_count, 0, current_count);
    return current;
}
