#pragma once

#ifdef _LIB
#define DLLAPI __declspec( dllexport )
#else
#define DLLAPI __declspec( dllimport )
#endif

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace profiler {
	using FuncID = void*;
	using TimeStamp = std::chrono::high_resolution_clock::time_point;
	struct FuncInfo {
		FuncID id = nullptr;
		char funcNameUndecorated[1024] = { {'\0'} };
		char funcNameDecorated[1024] = { {'\0'} };
		char fileName[1024] = { {'\0'} };
		int fileLine = 0;
	};
	struct StatsEntry {
		int invocationCount = 0;
		long long int nsTot = 0;
		long long int nsMin = 1000000000;
		long long int nsMax = 0;
	};
	struct FrameHistoryEntry {
		FuncID id = nullptr;
		TimeStamp time{};
	};
	DLLAPI void Enable();
	DLLAPI void Disable();
	DLLAPI void FrameStart();
	DLLAPI void FrameEnd();
	DLLAPI void ClearStats();
	DLLAPI const FuncInfo& GetFuncInfo(FuncID func);
	DLLAPI const std::unordered_map<FuncID, FuncInfo>& GetFuncTable();
	DLLAPI const std::unordered_map<FuncID, StatsEntry>& GetStats();
	DLLAPI const std::vector<FrameHistoryEntry>& GetFrameHistory();
	//
	bool __GetFuncInfo(FuncID func, profiler::FuncInfo& info);
}

// [NECESSARY] Since they're referenced inside 'hooks.asm'
extern "C" {
	void PEnter(profiler::FuncID func);
	void PExit(profiler::FuncID func);
};
