#include "threading.hpp"

#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // _WIN32

void threading::lockCurrentThread(int coreInd) {
#ifdef _WIN32
	SetThreadAffinityMask(GetCurrentThread(), 1ULL << coreInd);
#endif // _WIN32
}
