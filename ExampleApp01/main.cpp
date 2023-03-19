#include <cstdio>
#include <iostream>

#include "..\ProfilerLib\profilerlib.hpp"

__declspec(noinline) void testNoInline() {
    std::cout << "No-Inline\n";
}

class Class1 {
public:
    Class1() {
        std::cout << "Constructor#1\n";
    }
    void method1() {
        std::cout << "Meth#1\n";
    }
    void method2() {
        std::cout << "Meth#2\n";
        this->method1();
    }
    ~Class1() {
        std::cout << "Destructor#1\n";
    }
};

static void func1() {
    std::cout << "Func#1\n";
}

void func2() {
    std::cout << "Func#2\n";
    func1();
}

void func3(int a, int b) {
    std::cout << "Func#3\n";
}

void func4() {
    std::cout << "Func#4\n";
    for (int i = 0; i < 4; ++i)
        func1();
}

void rec(int d) {
    if (d == 0) return;
    std::cout << "Rec#1\n";
    rec(d - 1);
}

int main(int argc, char* argv[]) {
    std::cout << "[PROFILER ENABLED]\n";
    profiler::enable();
    printf("Printf Test\n");
    for (int i = 0; i < 5; ++i) {
        profiler::frame::start();
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
        auto lambda1 = [&]() { std::cout << "Lambda#1\n"; };
        auto lambda2 = [=]() { std::cout << "Lambda#2\n"; };
        auto lambda3 = []() { std::cout << "Lambda#3\n"; };
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
        profiler::frame::end();
    }
    profiler::disable();
    std::cout << "[PROFILER DISABLED]\n";
    func1();
    Class1 c12;
    return 0;
}
