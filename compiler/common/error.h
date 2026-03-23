#ifndef HPP_ERROR_H
#define HPP_ERROR_H

#include <stdbool.h>

typedef enum {
    ERR_LEXER,
    ERR_PARSER,
    ERR_TYPE,
    ERR_SEMA,
    ERR_CODEGEN,
    ERR_INTERNAL,
} ErrorCategory;

typedef struct {
    const char *filename;
    int line;
    int col;
} SourceLoc;

// Format: <file>:<line>:<col>: error[<category>]: <message>
void error_report(SourceLoc loc, ErrorCategory cat, const char *fmt, ...);

// Report and exit
_Noreturn void error_fatal(SourceLoc loc, ErrorCategory cat, const char *fmt, ...);

void error_init(void);
int  error_count(void);
bool error_has_errors(void);

// Get category string
const char *error_category_str(ErrorCategory cat);

#endif
