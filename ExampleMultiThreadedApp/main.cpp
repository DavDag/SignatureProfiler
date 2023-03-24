#include <iostream>
#include <thread>
#include <ppl.h>

#include "main_profiled.hpp"
#include "..\ProfilerLib\profilerlib.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Main: " << std::this_thread::get_id() << "\n";

    //////////////////////////////////////////////////
    {
        profiler::Enable();
        profiler::FrameStart();
        profiler::TimeStamp beg = profiler::Now();
        // ////////////////////////
        // std::vector<std::thread> threads;
        // for (int i = 0; i < THREAD_COUNT; ++i)
        //     threads.push_back(std::thread(yuppie, beg, i));
        // for (int i = 0; i < THREAD_COUNT; ++i)
        //     threads[i].join();
        ////////////////////////
        concurrency::parallel_for(0, THREAD_COUNT, [&beg](size_t index) {
            yuppie(beg, index);
        }, concurrency::static_partitioner());
        ////////////////////////
        profiler::FrameEnd();
        profiler::Disable();
    }
    
    //////////////////////////////////////////////////
    // profiler::LogHistoryCompact(profiler::GetFrameHistory());
    // printf("\n\n");
    // profiler::LogStatsCompact(profiler::GetStatsTable());
    // printf("\n\n");
    
    //////////////////////////////////////////////////
    for (int i = 0; i < THREAD_COUNT; ++i)
        std::cout << i << ": " << gResults[i].str() << "\n";
    std::cout << "\n\n";
    for (int i = 0; i < THREAD_COUNT; ++i)
        profiler::LogHistoryCompact(gHistories[i]);
    std::cout << "\n\n";

    return 0;
}
