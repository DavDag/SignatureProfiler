#include <cstdio>
#include <iostream>

#include "..\ProfilerLib\profilerlib.hpp"

//#define LOG(STR) std::cout << STR
#define LOG(STR)

__declspec(noinline) void testNoInline() {
    LOG("No-Inline\n");
}

class Class1 {
public:
    Class1() {
        LOG("Constructor#1\n");
    }
    void method1() {
        LOG("Meth#1\n");
    }
    void method2() {
        LOG("Meth#2\n");
        this->method1();
    }
    ~Class1() {
        LOG("Destructor#1\n");
    }
};

static void func1() {
    LOG("Func#1\n");
}

void func2() {
    LOG("Func#2\n");
    func1();
}

void func3(int a, int b) {
    LOG("Func#3\n");
}

void func4() {
    LOG("Func#4\n");
    for (int i = 0; i < 4; ++i)
        func1();
}

void rec(int d) {
    if (d == 0) return;
    LOG("Rec#1\n");
    rec(d - 1);
}

void mymain() {
    testNoInline();
    func1();
    func2();
    func3(10, 11);
    {
        Class1 c1;
        c1.method1();
        c1.method2();
    }
    rec(3);
    auto lambda1 = [&]() { LOG("Lambda#1\n"); };
    auto lambda2 = [=]() { LOG("Lambda#2\n"); };
    auto lambda3 = []() { LOG("Lambda#3\n"); };
    lambda1();
    lambda1();
    lambda2();
    lambda3();
    func4();
    Class1 c1;
    lambda2();
    func2();
    func1();
    func1();
    rec(5);
    c1.method2();
}

int main(int argc, char* argv[]) {
    auto mainStart = std::chrono::high_resolution_clock::now();
    ///////////////////////////////////////////////////////////////////////////////////
    LOG("[PROFILER ENABLED]\n");
    profiler::Enable();
    long long int profilerInvocations = 0;
    for (int i = 0; i < 100'000; ++i) {
        profiler::FrameStart();
        mymain();
        profilerInvocations += profiler::GetFrameHistory().size();
        profiler::FrameEnd();
    }
    profiler::LogHistory();
    profiler::Disable();
    LOG("[PROFILER DISABLED]\n");
    ///////////////////////////////////////////////////////////////////////////////////
    auto mainEnd = std::chrono::high_resolution_clock::now();
    auto mainDelta = std::chrono::duration_cast<std::chrono::microseconds>(mainEnd - mainStart);
    ///////////////////////////////////////////////////////////////////////////////////
    long long int timeSpentInsideFunctions = 0;
    const auto& stats = profiler::GetStats();
    for (const auto& p : stats) {
        auto funcId = p.first;
        const auto& data = p.second;
        const auto& funcInfo = profiler::GetFuncInfo(funcId);
        printf(
            "%-64.64s.%-4d | Max: %8lld (ns) | Min: %8lld (ns) | Avg: %8lld (ns) | Tot: %8lld (ns) | Count: %d\n",
            //funcInfo.fileName,
            funcInfo.funcNameUndecorated,
            funcInfo.fileLine,
            data.nsMax,
            data.nsMin,
            data.nsTot / data.invocationCount,
            data.nsTot,
            data.invocationCount
        );
        timeSpentInsideFunctions += data.nsTot;
    }
    ///////////////////////////////////////////////////////////////////////////////////
    printf("Main Tot (ns): %lld\n", mainDelta.count());
    printf("Profiler Invocation Count: %lld\n", profilerInvocations);
    printf("Profiler Time (ns): %lld\n", mainDelta.count() - timeSpentInsideFunctions);
    printf("Profiler Avg x Call (ns): %.4f\n", (mainDelta.count() - timeSpentInsideFunctions) / (double)profilerInvocations);
    return 0;
}
