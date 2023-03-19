#include "profilerlib_internal.hpp"

bool profiler::internal::enabled = false;
std::unordered_map<void*, const profiler::AddrInfo*> profiler::internal::database{};

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
	if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
		error();

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
		error();

	return true;
}

bool profiler::internal::getAddrInfo(void* address, AddrInfo* out) {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 1. Retrieve Symbol Info
	DWORD64 dwDisplacement1 = 0;
	CHAR buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { {0} };
	PSYMBOL_INFO pSymbolInfo = (PSYMBOL_INFO)buffer;
	pSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbolInfo->MaxNameLen = MAX_SYM_NAME-1;
	if (!SymFromAddr(GetCurrentProcess(), (DWORD64)address, &dwDisplacement1, pSymbolInfo))
		error();

	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 2. Retrieve Line Info
	DWORD dwDisplacement2 = 0;
	IMAGEHLP_LINE64 lineInfo{0};
	lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	if (!SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)address, &dwDisplacement2, &lineInfo))
		error();

	//////////////////////////////////////////////////////////////////////////////
	// 3. Fill result
	out->address = address;
	strcpy_s(out->fileName, lineInfo.FileName);
	strcpy_s(out->funcName, pSymbolInfo->Name);
	out->line = lineInfo.LineNumber;

	return true;
}

void profiler::internal::error() {
	int code = GetLastError();
	char errorMsg[4096] = { {'\0'} };
	errorAsString(code, errorMsg, 4096);
	fprintf(stderr, "Profiling error #%d: %s\n", code, errorMsg);
	::exit(code);
}

void profiler::internal::errorAsString(int errorCode, char* outbuffer, size_t outbuffersize) {
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		outbuffer,
		(DWORD)outbuffersize,
		NULL
	);
}
