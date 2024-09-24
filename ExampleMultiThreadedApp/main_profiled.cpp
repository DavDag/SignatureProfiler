#include "main_profiled.hpp"

#include <random>

std::stringstream gResults[THREAD_COUNT] = { {} };
profiler::FrameHistory gHistories[THREAD_COUNT] = { {} };

int fakeload(int load) {
    int acc = 1;
    for (int i = 0; i < 1000 * load; ++i) {
        acc += i % acc;
        int acc2 = 0;
        for (int j = 0; j < 1 * load; ++j) {
            acc2 += j * i;
        }
        acc += acc2;
    }
    return acc;
}

int yuppie(profiler::TimeStamp refBeg, size_t tIndex) {
    profiler::TimeStamp beg = profiler::Now();
    int acc = fakeload(1000);
    profiler::TimeStamp end = profiler::Now();
    profiler::DeltaUs delta = profiler::ComputeDelta(beg, end);
    profiler::DeltaUs deltaBeg = profiler::ComputeDelta(refBeg, beg);
    profiler::DeltaUs deltaEnd = profiler::ComputeDelta(refBeg, end);
    /////////////////////////////
    gHistories[tIndex] = profiler::FrameHistory(profiler::GetFrameHistory());
    gResults[tIndex]
        << std::this_thread::get_id()
        << " [" << deltaBeg << ", " << deltaEnd << "] "
        << delta << " (us)";
    return acc;
}

int yuppie_complex() {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 9999);
    int acc = 0;
    for (int r = 0; r < 1'000; r++) {
        int roll = distribution(generator);
        acc += roll;
        int acc2 = 0;
        for (int i = 0; i < 500'000 * (roll % 4); ++i) {
            acc2 += i;
        }
    }
    return acc;
}
