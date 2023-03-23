#include "profilerlib.hpp"

static bool gEnabled = false;
thread_local int gFrameHistoryIndex = 0;
thread_local std::vector<profiler::FrameHistoryEntry> gFrameHistory[2] = { {} };
thread_local std::unordered_map<profiler::FuncID, profiler::StatsEntry> gStatsDatabase{};
thread_local std::unordered_map<profiler::FuncID, profiler::FuncInfo> gInfoDatabase{};

struct StackEntry {
	profiler::FuncID func;
	profiler::TimeStamp start;
};
thread_local std::vector<StackEntry> gStack{};

//////////////////////////////////////////////////////////////////////////////

void PEnter(profiler::FuncID func) {
	if (!gEnabled) return;
	gFrameHistory[gFrameHistoryIndex].push_back({
		.id = func,
		.time = profiler::Now(),
		});
}

void PExit(profiler::FuncID func /* should be NULL */) {
	if (!gEnabled) return;
	gFrameHistory[gFrameHistoryIndex].push_back({
		.id = profiler::EmptyFuncID,
		.time = profiler::Now(),
		});
}

//////////////////////////////////////////////////////////////////////////////

bool profiler::Enable() {
	bool old = gEnabled;
	gEnabled = true;
	return old;
}

bool profiler::Disable() {
	bool old = gEnabled;
	gEnabled = false;
	return old;
}

void profiler::FrameStart() {
	if (!gEnabled) return;
	gFrameHistoryIndex = (gFrameHistoryIndex + 1) % 2;
	gFrameHistory[gFrameHistoryIndex].clear();
}

void profiler::FrameEnd() {
	if (!gEnabled) return;
	gStack.clear();
	for (const auto& e : gFrameHistory[gFrameHistoryIndex]) {
		if (e.id != EmptyFuncID) {
			gStack.push_back({ .func = e.id, .start = e.time });
		}
		else {
			if (gStack.size() == 0) continue;
			FuncID id = gStack[gStack.size() - 1].func;
			TimeStamp beg = gStack[gStack.size() - 1].start;
			TimeStamp end = e.time;
			DeltaNs delta = ComputeDelta(beg, end);
			gStack.pop_back();
			if (!gStatsDatabase.contains(id))
				gStatsDatabase.insert({ id, {} });
			profiler::StatsEntry& entry = gStatsDatabase.at(id);
			entry.invocationCount++;
			entry.nsMin = std::min(entry.nsMin, delta);
			entry.nsMax = std::max(entry.nsMax, delta);
			entry.nsTot += delta;
			entry.nsAvg = entry.nsTot / entry.invocationCount;
		}
	}
}

void profiler::ClearStats() {
	gStatsDatabase.clear();
}

const profiler::FuncInfo& profiler::GetFuncInfo(FuncID func) {
	if (!gInfoDatabase.contains(func)) {
		FuncInfo info{};
		__GetFuncInfo(func, info);
		gInfoDatabase.insert({ func, info });
	}
	return gInfoDatabase.at(func);
}

const std::unordered_map<profiler::FuncID, profiler::FuncInfo>& profiler::GetInfoTable() {
	return gInfoDatabase;
}

const profiler::StatsEntry& profiler::GetFuncStats(FuncID func) {
	return gStatsDatabase.at(func);
}

const std::unordered_map<profiler::FuncID, profiler::StatsEntry>& profiler::GetStatsTable() {
	return gStatsDatabase;
}

const std::vector<profiler::FrameHistoryEntry>& profiler::GetFrameHistory() {
	return gFrameHistory[((gFrameHistoryIndex - 1) + 2) % 2];
}

//////////////////////////////////////////////////////////////////////////////

void profiler::LogStats() {
	for (const auto& p : gStatsDatabase) {
		auto funcId = p.first;
		const auto& data = p.second;
		const auto& funcInfo = profiler::GetFuncInfo(funcId);
		printf(
			"%-64.64s.%-4d | Max: %8lld (ns) | Min: %8lld (ns) | Avg: %8lld (ns) | Tot: %8lld (ns) | Count: %d\n",
			funcInfo.funcName,
			funcInfo.fileLine,
			data.nsMax,
			data.nsMin,
			data.nsAvg,
			data.nsTot,
			data.invocationCount
		);
	}
}

void profiler::LogHistory() {
	std::stack<const FrameHistoryEntry*> callstack;
	for (const auto& e : gFrameHistory[gFrameHistoryIndex]) {
		if (e.id != EmptyFuncID) {
			callstack.push(&e);
			const auto& funcInfo = profiler::GetFuncInfo(e.id);
			printf(
				"%*.s[+] %-s.%-3d\n",
				(unsigned int)callstack.size() - 1,
				"",
				funcInfo.funcName,
				funcInfo.fileLine
			);
		}
		else {
			auto start = callstack.top()->time;
			const auto& funcInfo = profiler::GetFuncInfo(callstack.top()->id);
			callstack.pop();
			printf(
				"%*.s[-] %-s.%-3d, time: %llu (ns)\n",
				(unsigned int) callstack.size(),
				"",
				funcInfo.funcName,
				funcInfo.fileLine,
				(std::chrono::duration_cast<std::chrono::microseconds>(e.time - start)).count()
			);
		}
	}
}
