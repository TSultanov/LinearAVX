#pragma once

#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
void debug_print(const char* fmt, ...);
#ifdef __cplusplus
}
#endif

inline void waitForDebugger() {
    debug_print("PID %d, attach debugger and press any key...\n", getpid());
    getchar();
}