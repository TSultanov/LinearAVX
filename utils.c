#include "utils.h"
#include <stdarg.h>
#include <unistd.h>

void debug_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}