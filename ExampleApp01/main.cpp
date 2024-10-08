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
    LOG("[PROFILER ENABLED]\n");
    profiler::Enable();
    for (int i = 0; i < 100'000; ++i) {
        profiler::FrameStart();
        mymain();
        profiler::FrameEnd();
    }
    profiler::Disable();
    LOG("[PROFILER DISABLED]\n");
    printf("\n[HISTORY]\n");
    profiler::LogHistory(profiler::GetFrameHistory());
    printf("\n[STATS]\n");
    profiler::LogStats(profiler::GetStatsTable());
    return 0;
}
