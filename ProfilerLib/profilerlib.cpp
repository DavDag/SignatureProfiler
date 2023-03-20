#include "profilerlib.hpp"

static bool gEnabled = false;
thread_local std::vector<profiler::FrameHistoryEntry> gFrameHistory{};
thread_local std::unordered_map<profiler::FuncID, profiler::StatsEntry> gStats{};
thread_local std::unordered_map<profiler::FuncID, profiler::FuncInfo> gFuncDatabase{};

struct StackEntry {
	profiler::FuncID func;
	profiler::TimeStamp start;
};
thread_local std::vector<StackEntry> gStack{};

void PEnter(profiler::FuncID func) {
	if (gEnabled) {
		auto now = std::chrono::high_resolution_clock::now();
		gFrameHistory.push_back({.id = func, .time = now});
	}
}

void PExit(profiler::FuncID func /* should be NULL */) {
	if (gEnabled) {
		auto now = std::chrono::high_resolution_clock::now();
		gFrameHistory.push_back({ .id = NULL, .time = now });
	}
}

void profiler::Enable() {
	gEnabled = true;
}

void profiler::Disable() {
	gEnabled = false;
}

void profiler::FrameStart() {

}

void profiler::FrameEnd() {
	gStack.clear();
	for (const auto& e : gFrameHistory) {
		if (e.id != nullptr) {
			gStack.push_back({ .func = e.id, .start = e.time });
		}
		else {
			auto id = gStack[gStack.size() - 1].func;
			auto beg = gStack[gStack.size() - 1].start;
			gStack.pop_back();
			auto end = e.time;
			auto delta = std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
			if (!gStats.contains(id))
				gStats.insert({ id, {} });
			profiler::StatsEntry& entry = gStats.at(id);
			entry.invocationCount++;
			entry.nsMin = std::min(entry.nsMin, delta);
			entry.nsMax = std::max(entry.nsMax, delta);
			entry.nsTot += delta;
		}
	}
	gFrameHistory.clear();
}

void profiler::ClearStats() {
	gStats.clear();
}

const profiler::FuncInfo& profiler::GetFuncInfo(FuncID func) {
	if (!gFuncDatabase.contains(func)) {
		FuncInfo info{};
		__GetFuncInfo(func, info);
		gFuncDatabase.insert({ func, info });
	}
	return gFuncDatabase.at(func);
}

const std::unordered_map<profiler::FuncID, profiler::FuncInfo>& profiler::GetFuncTable() {
	return gFuncDatabase;
}

const std::unordered_map<profiler::FuncID, profiler::StatsEntry>& profiler::GetStats() {
	return gStats;
}

const std::vector<profiler::FrameHistoryEntry>& profiler::GetFrameHistory() {
	return gFrameHistory;
}
