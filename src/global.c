#include "global.h"

#include <stdarg.h>
#include <stdio.h>

bool verbose = false;

int printfv(const char *fmt, ...) {
    if (!verbose) return 1;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}