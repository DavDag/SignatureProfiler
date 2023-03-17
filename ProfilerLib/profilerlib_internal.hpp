#pragma once

#include "profilerlib.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imagehlp.h>
#include <unordered_map>

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID);

namespace profiler {
	namespace internal {
		extern bool enabled;
		extern ProfilingEventCallback enterEventCallback;
		extern ProfilingEventCallback exitEventCallback;
		extern ProfilingErrorCallback errorCallback;
		extern std::unordered_map<void*, ProfilingAddrInfo> database;
		//
		bool init(HMODULE hModule);
		bool exit();
		bool getAddrInfo(void* address, ProfilingAddrInfo& out);
		bool error();
		void errorAsString(int errorCode, char* outbuffer, size_t outbuffersize);
	}
}
