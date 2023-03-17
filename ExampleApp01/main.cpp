#include <iostream>

#include "profiling.hpp"

void func1() {
    std::cout << "===== [1] =====\n";
}

void func2() {
    std::cout << "===== [2] =====\n";
    func1();
}

void func3(int a, int b) {
    std::cout << "===== [3] =====\n";
}

int main(int argc, char* argv[]) {
    profiler::enable();
    profiler::setErrorCallback(HandleProfilingError);
    profiler::setEnterEventCallback(HandleEnterProfilingEvent);
    profiler::setExitEventCallback(HandleExitProfilingEvent);
    func1();
    func2();
    func3(10, 11);
    profiler::disable();
    func1();
    return 0;
}
