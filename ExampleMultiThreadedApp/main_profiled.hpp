#pragma once

#include "..\ProfilerLib\profilerlib.hpp"

constexpr int THREAD_COUNT = 4;
extern std::stringstream gResults[THREAD_COUNT];
extern profiler::FrameHistory gHistories[THREAD_COUNT];

int yuppie(profiler::TimeStamp refBeg, size_t tIndex);
int yuppie_complex();
