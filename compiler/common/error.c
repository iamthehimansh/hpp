#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static int g_error_count = 0;

void error_init(void) {
    g_error_count = 0;
}

int error_count(void) {
    return g_error_count;
}

bool error_has_errors(void) {
    return g_error_count > 0;
}

const char *error_category_str(ErrorCategory cat) {
    switch (cat) {
        case ERR_LEXER:    return "lexer";
        case ERR_PARSER:   return "parser";
        case ERR_TYPE:     return "type";
        case ERR_SEMA:     return "sema";
        case ERR_CODEGEN:  return "codegen";
        case ERR_INTERNAL: return "internal";
    }
    return "unknown";
}

void error_report(SourceLoc loc, ErrorCategory cat, const char *fmt, ...) {
    g_error_count++;
    fprintf(stderr, "%s:%d:%d: error[%s]: ",
            loc.filename ? loc.filename : "<unknown>",
            loc.line, loc.col, error_category_str(cat));
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

_Noreturn void error_fatal(SourceLoc loc, ErrorCategory cat, const char *fmt, ...) {
    g_error_count++;
    fprintf(stderr, "%s:%d:%d: error[%s]: ",
            loc.filename ? loc.filename : "<unknown>",
            loc.line, loc.col, error_category_str(cat));
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}
