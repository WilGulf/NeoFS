#include "global.h"

#include <stdarg.h>
#include <stdio.h>

bool verbose = false;

int printfv(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}