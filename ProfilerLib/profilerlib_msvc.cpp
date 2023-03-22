#include "profilerlib.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imagehlp.h>

#define error() __Error(__FUNCSIG__, __LINE__)

bool __Init(HMODULE hModule);
bool __Exit();
bool __Error(const char* func, const int line);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpvReserved) {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain

	// 
	if (reason == DLL_PROCESS_ATTACH) {
		if (!__Init(hModule)) {
			return FALSE;
		}
	}

	// 
	if (reason == DLL_PROCESS_DETACH) {
		if (!__Exit()) {
			return FALSE;
		}
	}

	// Returning FALSE fails the DLL load
	return TRUE;
}

bool __Init(HMODULE hModule) {
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

bool __Exit() {
	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/terminating-the-symbol-handler
	// 1. Terminate Symbol Handler
	if (!SymCleanup(GetCurrentProcess()))
		return error();

	//////////////////////////////////////////////////////////////////////////////
	// 2. Release memory

	return true;
}

bool __Error(const char* func, const int line) {
	int code = GetLastError();
	char errorMsg[4096] = { {'\0'} };
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMsg,
		(DWORD)4096,
		NULL
	);
	fprintf(stderr, "Profiling error#%d: %s\n\tat %s.%d\n", code, errorMsg, func, line);
	//::exit(code);
	return false;
}

void profiler::__GetFuncInfo(FuncID func, FuncInfo& info) {
	info.id = func;

	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 1. Retrieve Symbol Info
	DWORD64 dwDisplacement1 = 0;
	CHAR buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { {0} };
	PSYMBOL_INFO pSymbolInfo = (PSYMBOL_INFO)buffer;
	pSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbolInfo->MaxNameLen = MAX_SYM_NAME - 1;
	if (!SymFromAddr(GetCurrentProcess(), (DWORD64)func, &dwDisplacement1, pSymbolInfo)) {
		sprintf_s(info.funcNameExt, "SymFromAddr Error (%d)", GetLastError());
		sprintf_s(info.funcName, "SymFromAddr Error (%d)", GetLastError());
		// error();
	}
	else {
		strcpy_s(info.funcNameExt, pSymbolInfo->Name);
		//////////////////////////////////////////////////////////////////////////////
		// 2. Undecorated name
		CHAR undecoratedName[MAX_SYM_NAME] = { {'\0'} };
		if (UnDecorateSymbolName(pSymbolInfo->Name, undecoratedName, MAX_SYM_NAME, UNDNAME_COMPLETE) == NULL) {
			memset(undecoratedName, 0, MAX_SYM_NAME);
			sprintf_s(undecoratedName, "UnDecorateSymbolName Error (%d)", GetLastError());
			// error();
		}
		else
			strcpy_s(info.funcName, undecoratedName);
	}

	//////////////////////////////////////////////////////////////////////////////
	// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	// 3. Retrieve Line Info
	DWORD dwDisplacement2 = 0;
	IMAGEHLP_LINE64 lineInfo{};
	lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	if (!SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)func, &dwDisplacement2, &lineInfo)) {
		sprintf_s(info.fileName, "SymGetLineFromAddr64 (%d)", GetLastError());
		info.fileLine = 0;
		//error();
	}
	else {
		strcpy_s(info.fileName, lineInfo.FileName);
		info.fileLine = lineInfo.LineNumber;
	}
}