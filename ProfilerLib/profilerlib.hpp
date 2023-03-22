#pragma once

#ifdef _LIB
#define DLLAPI __declspec( dllexport )
#else
#define DLLAPI __declspec( dllimport )
#endif

#include <algorithm>
#include <vector>
#include <stack>
#include <unordered_map>
#include <chrono>

namespace profiler {
	// Functions
	using FuncID = void*;
	constexpr FuncID EmptyFuncID = nullptr;
	struct FuncInfo {
		FuncID id = EmptyFuncID;
		char funcName[1024] = { {'\0'} };
		char funcNameExt[1024] = { {'\0'} };
		char fileName[1024] = { {'\0'} };
		int fileLine = 0;
	};

	// Time
	using TimeStamp = std::chrono::high_resolution_clock::time_point;
	using DeltaNs = long long int;
	inline TimeStamp Now() noexcept {
		return std::chrono::high_resolution_clock::now();
	}
	inline DeltaNs ComputeDelta(TimeStamp beg, TimeStamp end) noexcept {
		return (std::chrono::duration_cast<std::chrono::microseconds>(end - beg)).count();
	}
	struct StatsEntry {
		DeltaNs nsTot = 0;
		DeltaNs nsMin = 1'000'000'000;
		DeltaNs nsMax = 0;
		int invocationCount = 0;
	};
	struct FrameHistoryEntry {
		FuncID id = nullptr;
		TimeStamp time{};
	};

	// Apis
	DLLAPI bool Enable();
	DLLAPI bool Disable();
	DLLAPI void FrameStart();
	DLLAPI void FrameEnd();
	DLLAPI void ClearStats();
	DLLAPI const FuncInfo& GetFuncInfo(FuncID func);
	DLLAPI const std::unordered_map<FuncID, FuncInfo>& GetFuncTable();
	DLLAPI const std::unordered_map<FuncID, StatsEntry>& GetStats();
	DLLAPI const std::vector<FrameHistoryEntry>& GetFrameHistory();
	
	// Utils
	DLLAPI void LogStats();
	DLLAPI void LogHistory();
	
	// Internals
	void __GetFuncInfo(FuncID func, profiler::FuncInfo& info);
}

// [NECESSARY] Since they're referenced inside 'hooks.asm'
extern "C" {
	void PEnter(profiler::FuncID func);
	void PExit(profiler::FuncID func);
};
