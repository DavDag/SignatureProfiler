#include "profilerlib_internal.hpp"

bool profiler::internal::enabled = false;
profiler::ProfilingEventCallback profiler::internal::enterEventCallback = nullptr;
profiler::ProfilingEventCallback profiler::internal::exitEventCallback = nullptr;
profiler::ProfilingErrorCallback profiler::internal::errorCallback = nullptr;
std::unordered_map<void*, profiler::ProfilingAddrInfo> profiler::internal::database{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpvReserved) {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain

	// 
	if (reason == DLL_PROCESS_ATTACH) {
		if (!profiler::internal::init(hModule)) {
			return FALSE;
		}
	}

	// 
	if (reason == DLL_PROCESS_DETACH) {
		if (!profiler::internal::exit()) {
			return FALSE;
		}
	}

	// Returning FALSE fails the DLL load
	return TRUE;
}

bool profiler::internal::init(HMODULE hModule) {
	//////////////////////////////////////////////////////////////////////////////
	// 1. Find module name at runtime (smt like: "C:\..\..\ProfilerLib.dll")
	char moduleName[MAX_PATH];
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((void*)hModule, &mbi, sizeof(mbi));
	GetModuleFileNameA((HMODULE)mbi.AllocationBase, moduleName, MAX_PATH);

	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/initializing-the-symbol-handler
	// 2. Initialize Symbol Handler
	if (!SymInitialize(GetCurrentProcess(), moduleName, TRUE))
		return error();

	//////////////////////////////////////////////////////////////////////////////
	// 3. Options
	SymSetOptions(SYMOPT_LOAD_LINES);

	return true;
}

bool profiler::internal::exit() {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/terminating-the-symbol-handler
	// 1. Terminate Symbol Handler
	if (!SymCleanup(GetCurrentProcess()))
		return error();

	return true;
}

bool profiler::internal::getAddrInfo(void* address, ProfilingAddrInfo& out) {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 1. Retrieve Symbol Info
	TCHAR buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { {'\0'} };
	PSYMBOL_INFO pSymbolInfo = (PSYMBOL_INFO)buffer;
	pSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbolInfo->MaxNameLen = MAX_SYM_NAME;
	if (!SymFromAddr(GetCurrentProcess(), (DWORD64)address, NULL, pSymbolInfo))
		return error();

	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 2. Retrieve Line Info
	DWORD dwDisplacement;
	IMAGEHLP_LINE64 lineInfo;
	lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	if (!SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)address, &dwDisplacement, &lineInfo))
		return error();

	//////////////////////////////////////////////////////////////////////////////
	// 3. Fill result
	out.address = address;
	strcpy_s(out.fileName, lineInfo.FileName);
	strcpy_s(out.funcName, pSymbolInfo->Name);
	out.line = lineInfo.LineNumber;

	return true;
}

bool profiler::internal::error() {
	//////////////////////////////////////////////////////////////////////////////
	// 1. Retrieve error code & execute callback
	int code = GetLastError();
	if (profiler::internal::errorCallback) {
		profiler::internal::errorCallback(code);
	}
	printf("Error: %d\n", code);
	// Syntactic sugar to enable single line returns
	return false;
}

void profiler::internal::errorAsString(int errorCode, char* outbuffer, size_t outbuffersize) {
	//////////////////////////////////////////////////////////////////////////////
	// 1. Retrieve message using win32 apis
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		outbuffer,
		outbuffersize,
		NULL
	);
}
