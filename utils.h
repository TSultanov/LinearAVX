#pragma once

#include <stdio.h>
#include <unistd.h>

inline void waitForDebugger() {
    printf("PID %d, attach debugger and press any key...\n", getpid());
    getchar();
}