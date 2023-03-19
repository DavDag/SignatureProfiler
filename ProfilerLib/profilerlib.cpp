#include "profilerlib.hpp"
#include "profilerlib_internal.hpp"

#include <chrono>

struct ProfilingStackEntry {
	const profiler::AddrInfo* info = nullptr;
	std::chrono::high_resolution_clock::time_point start = {};
};

struct ProfilingStats {
	int count = 0;
	long long int nsTot = 0;
	long long int nsMin = 1000000000;
	long long int nsMax = 0;
};

thread_local int level = 0;
thread_local ProfilingStackEntry stack[256] = { {nullptr} };
thread_local std::unordered_map<const profiler::AddrInfo*, ProfilingStats> stats{};

void PEnter(void* address) {
	if (profiler::internal::enabled) {
		if (!profiler::internal::database.contains(address)) {
			profiler::AddrInfo* info = new profiler::AddrInfo();
			profiler::internal::getAddrInfo(address, info);
			profiler::internal::database.insert({ address, info });
		}
		const profiler::AddrInfo& info = *profiler::internal::database.at(address);
		//
		auto now = std::chrono::high_resolution_clock::now();
		stack[level] = { .info = &info, .start = now };
		//printf("%*.s+ %s.%d\n", level, "", info.funcName, info.line);
		level++;
	}
}

void PExit(void* address) {
	if (profiler::internal::enabled) {
		/*
		if (!profiler::internal::database.contains(address)) {
			profiler::AddrInfo info;
			profiler::internal::getAddrInfo(address, info);
			profiler::internal::database.insert({ address, info });
		}
		const profiler::AddrInfo& info = profiler::internal::database.at(address);
		*/
		//
		level--;
		const profiler::AddrInfo& info = *stack[level].info;
		auto now = std::chrono::high_resolution_clock::now();
		auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - stack[level].start).count();
		// printf("%*.s- %s.%d [%lld ns]\n", level, "", info.funcName, info.line, delta);
		//
		if (!stats.contains(&info))
			stats.insert({ &info, {} });
		ProfilingStats& entry = stats.at(&info);
		entry.count++;
		entry.nsMin = min(entry.nsMin, delta);
		entry.nsMax = max(entry.nsMax, delta);
		entry.nsTot += delta;
	}
}

void profiler::enable() {
	profiler::internal::enabled = true;
}

void profiler::disable() {
	profiler::internal::enabled = false;
}

void profiler::frame::start() {
	//profiler::internal::enabled = false;
	level = 0;
	//profiler::internal::enabled = true;
}

void profiler::frame::end() {
	//profiler::internal::enabled = false;
	printf("------------------------------------------------------------------------------------------\n");
	printf("                Func                 | Min (ns) | Max (ns) | Avg (ns) | Tot (ns) | #calls \n");
	for (const auto& p : stats) {
		const auto& info = *p.first;
		const auto& data = p.second;
		printf(
			"%-32.32s.%-3d | %8lld | %8lld | %8lld | %8lld | %6d \n",
			info.funcName,
			info.line,
			data.nsMin,
			data.nsMax,
			data.nsTot / data.count,
			data.nsTot,
			data.count
		);
	}
	printf("------------------------------------------------------------------------------------------\n");
	//stats.clear();
	//profiler::internal::enabled = true;
}

/*
                Func                 | Min (ns) | Max (ns) | Avg (ns) | Tot (ns) | #calls
`main'::`4'::<lambda_3>::operato.69  |       21 |       21 |       21 |       21 |      1
std::_Narrow_char_traits<char,in.402 |        0 |        0 |        0 |        0 |      2
std::basic_ostream<char,std::cha.70  |        0 |        0 |        0 |        0 |      2
std::operator<<<std::char_traits.761 |       22 |       28 |       25 |       50 |      2
std::basic_ostream<char,std::cha.91  |        0 |        1 |        0 |        1 |      2
Class1::method1                 .15  |       19 |       27 |       22 |       68 |      3
std::basic_ostream<char,std::cha.123 |        0 |        0 |        0 |        0 |      2
std::basic_ostream<char,std::cha.77  |        0 |        0 |        0 |        0 |      2
std::basic_ostream<char,std::cha.108 |        0 |        4 |        2 |        4 |      2
__local_stdio_printf_options    .90  |        0 |        0 |        0 |        0 |     35
_vfprintf_l                     .644 |       18 |       42 |       22 |      781 |     35
printf                          .956 |       18 |       43 |       22 |      800 |     35
Class1::~Class1                 .22  |       20 |       34 |       27 |       54 |      2
func1                           .27  |       19 |       43 |       23 |      213 |      9
func2                           .31  |       43 |       48 |       45 |       91 |      2
func3                           .36  |       23 |       23 |       23 |       23 |      1
Class1::Class1                  .12  |       23 |       35 |       29 |       58 |      2
Class1::method2                 .18  |       39 |       54 |       46 |       93 |      2
rec                             .46  |        0 |      102 |       45 |      450 |     10
`main'::`4'::<lambda_1>::operato.67  |       24 |       24 |       24 |       48 |      2
`main'::`4'::<lambda_2>::operato.68  |       26 |       27 |       26 |       53 |      2
func4                           .40  |      123 |      123 |      123 |      123 |      1
*/
