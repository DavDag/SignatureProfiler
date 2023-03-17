#pragma once

#ifdef _LIB
#define DLLAPI __declspec( dllexport )
#else
#define DLLAPI __declspec( dllimport )
#endif

namespace profiler {
	struct ProfilingAddrInfo {
		void* address = nullptr;
		char funcName[1024] = { {'\0'} };
		char fileName[1024] = { {'\0'} };
		int line = 0;
	};
	typedef void (*ProfilingEventCallback)(const ProfilingAddrInfo& info);
	typedef void (*ProfilingErrorCallback)(int errorCode);
	void DLLAPI enable();
	void DLLAPI disable();
	void DLLAPI setEnterEventCallback(ProfilingEventCallback callback);
	void DLLAPI setExitEventCallback(ProfilingEventCallback callback);
	void DLLAPI setErrorCallback(ProfilingErrorCallback callback);
	void errorAsString(int errorCode, char* outbuffer, size_t outbuffersize);
}

// [NECESSARY] Since they're referenced inside 'hooks.asm'
extern "C" void DLLAPI PEnter(void* address);
extern "C" void DLLAPI PExit(void* address);
