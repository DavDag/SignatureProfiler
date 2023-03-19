#pragma once

#ifdef _LIB
#define DLLAPI __declspec( dllexport )
#else
#define DLLAPI __declspec( dllimport )
#endif

namespace profiler {
	struct AddrInfo {
		void* address = nullptr;
		char funcName[1024] = { {'\0'} };
		char fileName[1024] = { {'\0'} };
		int line = 0;
	};
	void DLLAPI enable();
	void DLLAPI disable();
	namespace frame {
		void DLLAPI start();
		void DLLAPI end();
	}
}

// [NECESSARY] Since they're referenced inside 'hooks.asm'
extern "C" void DLLAPI PEnter(void* address);
extern "C" void DLLAPI PExit(void* address);
