#pragma once

#include "..\ProfilerLib\profilerlib.hpp"

void HandleProfilingError(int errorCode);
void HandleEnterProfilingEvent(const profiler::ProfilingAddrInfo& info);
void HandleExitProfilingEvent(const profiler::ProfilingAddrInfo& info);
