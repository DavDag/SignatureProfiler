#include "hwinfo.hpp"

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <imgui.h>
#include <glm/glm.hpp>

#include <thread>
#include <vector>
#include <array>
#include <iostream>

#define BYTES_TO_MB_DOUBLE (1024.0 * 1024.0)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <psapi.h>
#include <winternl.h>
#include <pdh.h>
#include <pdhmsg.h>
#endif

#ifdef _WIN32
HANDLE hCurrentProcess;
DWORD currentProcessId;
PDH_HQUERY gpuUsageQuery;
HCOUNTER gpuUsageCounter;
PDH_HQUERY gpuMemUsageQuery;
HCOUNTER gpuMemDedicatedUsageCounter;
HCOUNTER gpuMemSharedUsageCounter;
#endif // _WIN32

// ====================================
// Extra
// ====================================

int hwinfo::extra::pid() {
    static bool __computed = false;
    static int result = 0;
    if (__computed) return result;
    __computed = true;
    //
#ifdef _WIN32
    result = currentProcessId;
#endif // _WIN32
    return result;
}

int hwinfo::extra::runningCoreInd() {
    int result = 0;
#ifdef _WIN32
    DWORD pnum = GetCurrentProcessorNumber();
    result = pnum;
#endif // _WIN32
    return result;
}

// ====================================
// OpenGL
// ====================================

const char* hwinfo::opengl::version() {
    static bool __computed = false;
    static const char* result;
    if (__computed) return result;
    __computed = true;
    //
    result = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    return result;
}

// ====================================
// GPU
// ====================================

const char* hwinfo::gpu::vendor() {
    static bool __computed = false;
    static const char* result;
    if (__computed) return result;
    __computed = true;
    //
    result = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    return result;
}

const char* hwinfo::gpu::renderer() {
    static bool __computed = false;
    static const char* result;
    if (__computed) return result;
    __computed = true;
    //
    result = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    return result;
}

bool hwinfo::gpu::isIntel() {
    static bool __computed = false;
    static bool result = false;
    if (__computed) return result;
    __computed = false;
    //
    result = strcmp("Intel", hwinfo::gpu::vendor());
    return result;
}

bool hwinfo::gpu::isNVidia() {
    static bool __computed = false;
    static bool result = false;
    if (__computed) return result;
    __computed = false;
    //
    result = strcmp("NVidia", hwinfo::gpu::vendor());
    return result;
}

bool hwinfo::gpu::isAMD() {
    static bool __computed = false;
    static bool result = false;
    if (__computed) return result;
    __computed = false;
    //
    result = strcmp("AMD", hwinfo::gpu::vendor());
    return result;
}

double hwinfo::gpu::usage() {
    double result = 0;
#ifdef _WIN32
    static DWORD bufferSize = sizeof(PDH_FMT_COUNTERVALUE_ITEM) * 128, itemCount = 0;
    static PDH_FMT_COUNTERVALUE_ITEM* pdhItems = (PDH_FMT_COUNTERVALUE_ITEM*) new unsigned char[bufferSize];
    // https://askldjd.wordpress.com/2011/01/05/a-pdh-helper-class-cpdhquery/
    PDH_STATUS Status = ERROR_SUCCESS;
    double percent = 0;
    while(true && gpuUsageQuery) {
        Status = PdhCollectQueryData(gpuUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        //DWORD bufferSize = 0, itemCount = 0;
        //PdhGetFormattedCounterArray(gpuUsageCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
        //if (Status != PDH_MORE_DATA) break;
        //
        PdhGetFormattedCounterArray(gpuUsageCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, pdhItems);
        if (Status != ERROR_SUCCESS) break;
        percent = 0.0;
        for (unsigned int i = 0; i < itemCount; i++)
            percent += pdhItems[i].FmtValue.doubleValue;
        //
        break;
    }
    if (Status != ERROR_SUCCESS && Status != PDH_NO_DATA) printf("PDH error: 0x%x", Status);
    result = percent;
#endif // _WIN32
    return result;
}

double hwinfo::gpu::usageMb() {
    double result = 0;
#ifdef _WIN32
    PDH_FMT_COUNTERVALUE DedicatedValue, SharedValue;
    PDH_STATUS Status = ERROR_SUCCESS;
    double usage = 0;
    while (true && gpuMemUsageQuery) {
        Status = PdhCollectQueryData(gpuMemUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        PdhGetFormattedCounterValue(gpuMemDedicatedUsageCounter, PDH_FMT_LONG, NULL, &DedicatedValue);
        if (Status != ERROR_SUCCESS) break;
        usage += DedicatedValue.longValue / BYTES_TO_MB_DOUBLE;
        //
        PdhGetFormattedCounterValue(gpuMemSharedUsageCounter, PDH_FMT_LONG, NULL, &SharedValue);
        if (Status != ERROR_SUCCESS) break;
        usage += SharedValue.longValue / BYTES_TO_MB_DOUBLE;
        //
        break;
    }
    if (Status != ERROR_SUCCESS && Status != PDH_NO_DATA) printf("PDH error: 0x%x", Status);
    result = usage;
#endif // _WIN32
    return result;
}

double hwinfo::gpu::availableMb() {
    double result = 0;
    // TODO:
    return result;
}

double hwinfo::gpu::physicalTotMb() {
    static bool __computed = false;
    static double result = 0;
    if (__computed) return result;
    __computed = false;
    // TODO:
    return result;
}

// ====================================
// CPU
// ====================================

unsigned int hwinfo::cpu::threadCount() {
    static bool __computed = false;
    static int result;
    if (__computed) return result;
    __computed = true;
    //
    result = std::thread::hardware_concurrency();
    return result;
}

const char* hwinfo::cpu::vendor() {
    static bool __computed = false;
    static char result[32];
    if (__computed) return result;
    __computed = true;
#ifdef _WIN32
    // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
    int nIds;
    std::array<int, 4> cpui;
    std::vector<std::array<int, 4>> data;
    //
    __cpuid(cpui.data(), 0);
    nIds = cpui[0];
    for (int i = 0; i <= nIds; ++i) {
        __cpuidex(cpui.data(), i, 0);
        data.push_back(cpui);
    }
    //
    char vendor[32];
    memset(vendor, 0, sizeof(vendor));
    *reinterpret_cast<int*>(vendor) = data[0][1];
    *reinterpret_cast<int*>(vendor + 4) = data[0][3];
    *reinterpret_cast<int*>(vendor + 8) = data[0][2];
    //
    memcpy(result, vendor, sizeof(char) * 32);
#endif // _WIN32
    return result;
}

const char* hwinfo::cpu::brand() {
    static bool __computed = false;
    static char result[64] = "<undefined>";
    if (__computed) return result;
    __computed = true;
#ifdef _WIN32
    // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
    int nExIds;
    std::array<int, 4> cpui;
    std::vector<std::array<int, 4>> extdata;
    //
    __cpuid(cpui.data(), 0x80000000);
    nExIds = cpui[0];
    for (int i = 0x80000000; i <= nExIds; ++i) {
        __cpuidex(cpui.data(), i, 0);
        extdata.push_back(cpui);
    }
    //
    if (nExIds >= 0x80000004) {
        static char brand[64];
        memset(brand, 0, sizeof(brand));
        memcpy(brand, extdata[2].data(), sizeof(cpui));
        memcpy(brand + 16, extdata[3].data(), sizeof(cpui));
        memcpy(brand + 32, extdata[4].data(), sizeof(cpui));
        memcpy(result, brand, sizeof(char) * 64);
    }
#endif // _WIN32
    return result;
}

double hwinfo::cpu::usage(double cores[]) {
    double result = 0;
    memset(cores, 0, sizeof(double) * hwinfo::cpu::threadCount());
#if _WIN32
    // https://stackoverflow.com/questions/53306819/accurate-system-cpu-usage-in-windows
    static PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION lastValues = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[hwinfo::cpu::threadCount()];
    static PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION newValues = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[hwinfo::cpu::threadCount()];
    //
    ULONG size;
    NtQuerySystemInformation(SystemProcessorPerformanceInformation, newValues, sizeof(newValues[0]) * hwinfo::cpu::threadCount(), &size);
    double percent = 0.0;
    for (DWORD i = 0; i < hwinfo::cpu::threadCount(); ++i) {
        double current_percent = double(newValues[i].IdleTime.QuadPart - lastValues[i].IdleTime.QuadPart);
        current_percent /= ((newValues[i].KernelTime.QuadPart + newValues[i].UserTime.QuadPart) - (lastValues[i].KernelTime.QuadPart + lastValues[i].UserTime.QuadPart));
        current_percent = 1.0 - current_percent;
        current_percent *= 100.0;
        cores[i] = current_percent;
        percent += current_percent;
    }
    //
    memcpy(lastValues, newValues, sizeof(newValues[0]) * hwinfo::cpu::threadCount());
    result = percent / hwinfo::cpu::threadCount();
#endif // _WIN32
    return result;
}

// ====================================
// Memory
// ====================================

double hwinfo::mem::usageMb() {
    double result = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    int succeded = (GetProcessMemoryInfo(hCurrentProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)) != 0);
    result = (succeded) ? pmc.PrivateUsage / BYTES_TO_MB_DOUBLE : 0;
#endif // _WIN32
    return result;
}

double hwinfo::mem::availableMb() {
    double result = 0;
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    result = memInfo.ullAvailPhys / BYTES_TO_MB_DOUBLE;
#endif // _WIN32
    return result;
}

double hwinfo::mem::physicalTotMb() {
    static bool __computed = false;
    static double result = 0;
    if (__computed) return result;
    __computed = true;
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    result = memInfo.ullTotalPhys / BYTES_TO_MB_DOUBLE;
#endif // _WIN32
    return result;
}

// ====================================
// Dependencies
// ====================================

const char* hwinfo::deps::glfwVersion() {
    static bool __computed = false;
    static char result[64];
    if (__computed) return result;
    __computed = true;
    //
    strcpy_s(result, 64, glfwGetVersionString());
    return result;
}

const char* hwinfo::deps::glewVersion() {
    static bool __computed = false;
    static char result[64];
    if (__computed) return result;
    __computed = true;
    //
    strcpy_s(result, 64, (const char*) glewGetString(GLEW_VERSION));
    return result;
}

const char* hwinfo::deps::imguiVersion() {
    static bool __computed = false;
    static char result[64];
    if (__computed) return result;
    __computed = true;
    //
    strcpy_s(result, 64, IMGUI_VERSION);
    return result;
}

const char* hwinfo::deps::glmVersion() {
    static bool __computed = false;
    static char result[64];
    if (__computed) return result;
    __computed = true;
    //
    strcpy_s(result, 64, "0.9.9.8");
    return result;
}

// ====================================
// Internal
// ====================================

void hwinfo::init() {
#ifdef _WIN32
    hCurrentProcess = GetCurrentProcess();
    currentProcessId = GetCurrentProcessId();
    //
    PDH_STATUS Status;
    while (true) {
        Status = PdhOpenQuery(NULL, NULL, &gpuUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        WCHAR gpuUsageQueryBuffer[PDH_MAX_COUNTER_PATH];
        wsprintf(gpuUsageQueryBuffer, L"\\GPU Engine(pid_%d*)\\Utilization Percentage", currentProcessId);
        Status = PdhAddCounter(gpuUsageQuery, gpuUsageQueryBuffer, 0, &gpuUsageCounter);
        if (Status != ERROR_SUCCESS) break;
        //
        Status = PdhCollectQueryData(gpuUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        break;
    }
    if (Status != ERROR_SUCCESS && Status != PDH_NO_DATA) printf("PDH error: 0x%x\n", Status);
    //
    Status = ERROR_SUCCESS;
    while (true) {
        Status = PdhOpenQuery(NULL, NULL, &gpuMemUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        WCHAR gpuMemUsageQueryBuffer[PDH_MAX_COUNTER_PATH];
        wsprintf(gpuMemUsageQueryBuffer, L"\\GPU Process Memory(pid_%d*)\\Dedicated Usage", currentProcessId);
        Status = PdhAddCounter(gpuMemUsageQuery, gpuMemUsageQueryBuffer, 0, &gpuMemDedicatedUsageCounter);
        wsprintf(gpuMemUsageQueryBuffer, L"\\GPU Process Memory(pid_%d*)\\Shared Usage", currentProcessId);
        Status = PdhAddCounter(gpuMemUsageQuery, gpuMemUsageQueryBuffer, 0, &gpuMemSharedUsageCounter);
        if (Status != ERROR_SUCCESS) break;
        //
        Status = PdhCollectQueryData(gpuMemUsageQuery);
        if (Status != ERROR_SUCCESS) break;
        //
        break;
    }
    if (Status != ERROR_SUCCESS && Status != PDH_NO_DATA) printf("PDH error: 0x%x\n", Status);
#endif // _WIN32
}

void hwinfo::exit() {
#if _WIN32
    if (gpuUsageQuery) PdhCloseQuery(gpuUsageQuery);
    if (gpuMemUsageQuery) PdhCloseQuery(gpuMemUsageQuery);
#endif // _WIN32
}
