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
		extern std::unordered_map<void*, const AddrInfo*> database;
		//
		bool init(HMODULE hModule);
		bool exit();
		bool getAddrInfo(void* address, AddrInfo* out);
		void error();
		void errorAsString(int code, char* outbuffer, size_t outbuffersize);
	}
}
