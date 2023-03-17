///////////////////////////////////////////////////////////////////////////////////
//
// A separated compilation unit is required to customize compiling options,
// disabling /GH and /Gh (to ensure no recursive behaviours).
//
///////////////////////////////////////////////////////////////////////////////////

#include "profiling.hpp"
#include <stdio.h>
#include <stdlib.h>

static int level = 0;

void HandleProfilingError(int errorCode) {
    char errorMsg[4096] = { {'\0'} };
    fprintf(stderr, "Profiling error: %s (%d)\n", errorMsg, errorCode);
    exit(errorCode);
}

void HandleEnterProfilingEvent(const profiler::ProfilingAddrInfo& info) {
    printf("%*.s+ %p: %s.%d\n", level, "", info.address, info.funcName, info.line);
    level++;
}

void HandleExitProfilingEvent(const profiler::ProfilingAddrInfo& info) {
    level--;
    printf("%*.s- %p: %s.%d\n", level, "", info.address, info.funcName, info.line);
}
