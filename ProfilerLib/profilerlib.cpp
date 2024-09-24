#include "profilerlib.hpp"

static bool gEnabled = false;
thread_local profiler::InfoTable gInfoDatabase{};
thread_local profiler::StatsTable gStatsDatabase{};
thread_local int gFrameHistoryIndex = 0;
thread_local profiler::FrameHistory gFrameHistory[2] = { {} };

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
			DeltaUs delta = ComputeDelta(beg, end);
			gStack.pop_back();
			if (!gStatsDatabase.contains(id))
				gStatsDatabase.insert({ id, {} });
			profiler::FuncStats& entry = gStatsDatabase.at(id);
			entry.invocationCount++;
			entry.usMin = std::min(entry.usMin, delta);
			entry.usMax = std::max(entry.usMax, delta);
			entry.usTot += delta;
			entry.usAvg = entry.usTot / entry.invocationCount;
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

const profiler::InfoTable& profiler::GetInfoTable() {
	return gInfoDatabase;
}

const profiler::FuncStats& profiler::GetFuncStats(FuncID func) {
	return gStatsDatabase.at(func);
}

const profiler::StatsTable& profiler::GetStatsTable() {
	return gStatsDatabase;
}

const profiler::FrameHistory& profiler::GetFrameHistory() {
	return gFrameHistory[((gFrameHistoryIndex - 1) + 2) % 2];
}

//////////////////////////////////////////////////////////////////////////////

inline profiler::TimeStamp profiler::Now() noexcept {
	return std::chrono::high_resolution_clock::now();
}

inline profiler::DeltaUs profiler::ComputeDelta(TimeStamp beg, TimeStamp end) noexcept {
	return (std::chrono::duration_cast<std::chrono::microseconds>(end - beg)).count();
}

//////////////////////////////////////////////////////////////////////////////

void profiler::LogStats(const StatsTable& stats) {
	std::vector<std::pair<FuncID, FuncStats>> list(stats.begin(), stats.end());
	std::sort(list.begin(), list.end(),
		[](const std::pair<FuncID, FuncStats>& a, const std::pair<FuncID, FuncStats>& b) {
			return (a.second.usTot > b.second.usTot);
		});
	for (const auto& p : list) {
		auto funcId = p.first;
		const auto& data = p.second;
		const auto& funcInfo = profiler::GetFuncInfo(funcId);
		printf(
			"%-64.64s.%-4d | Max: %8lld (us) | Min: %8lld (us) | Avg: %8lld (us) | Tot: %8lld (us) | Count: %d\n",
			funcInfo.funcName,
			funcInfo.fileLine,
			data.usMax,
			data.usMin,
			data.usAvg,
			data.usTot,
			data.invocationCount
		);
	}
}

void profiler::LogStatsCompact(const StatsTable& stats) {
	std::vector<std::pair<FuncID, FuncStats>> list(stats.begin(), stats.end());
	std::sort(list.begin(), list.end(),
		[](const std::pair<FuncID, FuncStats>& a, const std::pair<FuncID, FuncStats>& b) {
			return (a.second.usTot > b.second.usTot);
		});
	for (const auto& p : list) {
		auto funcId = p.first;
		const auto& data = p.second;
		const auto& funcInfo = profiler::GetFuncInfo(funcId);
		printf(
			"%-32.32s.%-4d | Avg: %8lld (us) | Count: %d\n",
			funcInfo.funcName,
			funcInfo.fileLine,
			data.usAvg,
			data.invocationCount
		);
	}
}

void profiler::LogHistory(const FrameHistory& history) {
	std::stack<int> callstack;
	for (int i = 0; i < history.size(); ++i) {
		const auto& e = history[i];
		if (e.id != EmptyFuncID) {
			callstack.push(i);
			const auto& funcInfo = profiler::GetFuncInfo(e.id);
			printf(
				"%*.s[+] %-s.%-3d\n",
				((unsigned int)callstack.size() - 1) * 2,
				"",
				funcInfo.funcName,
				funcInfo.fileLine
			);
		}
		else {
			const auto& startEv = history[callstack.top()];
			const auto& funcInfo = profiler::GetFuncInfo(startEv.id);
			printf(
				"%*.s[-] %-s.%03d, time: %llu (us)\n",
				(unsigned int)(callstack.size() - 1) * 2,
				"",
				funcInfo.funcName,
				funcInfo.fileLine,
				ComputeDelta(startEv.time, e.time)
			);
			callstack.pop();
		}
	}
}

void profiler::LogHistoryCompact(const FrameHistory& history) {
	std::stack<int> callstack;
	for (int i = 0; i < history.size(); ++i) {
		const auto& e = history[i];
		if (e.id != EmptyFuncID) {
			callstack.push(i);
			const auto& funcInfo = profiler::GetFuncInfo(e.id);
			printf(
				"%*.s[+] %-s.%-3d\n",
				(unsigned int)(callstack.size() - 1) * 2,
				"",
				funcInfo.funcName,
				funcInfo.fileLine
			);
		}
		else {
			callstack.pop();
		}
	}
}
