/* Thin C bridge — varargs stay in C, error_category_str from H++ */
#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static int g_error_count = 0;

void error_init(void) { g_error_count = 0; }
int  error_count(void) { return g_error_count; }
bool error_has_errors(void) { return g_error_count > 0; }

void error_report(SourceLoc loc, ErrorCategory cat, const char *fmt, ...) {
    g_error_count++;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s:%d:%d: error[%s]: %s\n",
            loc.filename ? loc.filename : "<unknown>",
            loc.line, loc.col, error_category_str(cat), buf);
}

_Noreturn void error_fatal(SourceLoc loc, ErrorCategory cat, const char *fmt, ...) {
    g_error_count++;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s:%d:%d: error[%s]: %s\n",
            loc.filename ? loc.filename : "<unknown>",
            loc.line, loc.col, error_category_str(cat), buf);
    exit(1);
}
