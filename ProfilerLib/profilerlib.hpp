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

	// Time
	using TimeStamp = std::chrono::high_resolution_clock::time_point;
	using DeltaUs = long long int;

	// Structs
	struct FuncInfo {
		FuncID id = EmptyFuncID;
		char funcName[1024] = { {'\0'} };
		char funcNameExt[1024] = { {'\0'} };
		char fileName[1024] = { {'\0'} };
		size_t funcNameLen = 0;
		size_t funcNameExtLen = 0;
		size_t fileNameLen = 0;
		int fileLine = 0;
	};
	using InfoTable = std::unordered_map<FuncID, FuncInfo>;
	struct FuncStats {
		DeltaUs usTot = 0;
		DeltaUs usMin = 1'000'000'000;
		DeltaUs usMax = 0;
		DeltaUs usAvg = 0;
		int invocationCount = 0;
	};
	using StatsTable = std::unordered_map<FuncID, FuncStats>;
	struct FrameHistoryEntry {
		FuncID id = nullptr;
		TimeStamp time{};
	};
	using FrameHistory = std::vector<profiler::FrameHistoryEntry>;

	// Apis
	DLLAPI bool Enable();
	DLLAPI bool Disable();
	DLLAPI void FrameStart();
	DLLAPI void FrameEnd();
	DLLAPI void ClearStats();
	DLLAPI const FuncInfo& GetFuncInfo(FuncID func);
	DLLAPI const InfoTable& GetInfoTable();
	DLLAPI const FuncStats& GetFuncStats(FuncID func);
	DLLAPI const StatsTable& GetStatsTable();
	DLLAPI const FrameHistory& GetFrameHistory();

	// Utils
	DLLAPI void LogStats(const StatsTable& stats);
	DLLAPI void LogStatsCompact(const StatsTable& stats);
	DLLAPI void LogHistory(const FrameHistory& history);
	DLLAPI void LogHistoryCompact(const FrameHistory& history);

	// Extra
	using CRC32 = unsigned int;
	DLLAPI CRC32 ComputeCRC32(const char* data, int len, CRC32 crc = 0);
	constexpr CRC32 __ComputeCRC32(const char* data, int len, CRC32 crc = 0);
	DLLAPI inline TimeStamp Now() noexcept;
	DLLAPI inline DeltaUs ComputeDelta(TimeStamp beg, TimeStamp end) noexcept;
	
	// Internals
	void __GetFuncInfo(FuncID func, FuncInfo& info);
}

// [NECESSARY] Since they're referenced inside 'hooks.asm'
extern "C" {
	void PEnter(profiler::FuncID func);
	void PExit(profiler::FuncID func);
};
