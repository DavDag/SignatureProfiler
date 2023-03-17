#include "profilerlib.hpp"
#include "profilerlib_internal.hpp"

void PEnter(void* address) {
	if (profiler::internal::enabled) {
		profiler::ProfilingAddrInfo info;
		profiler::internal::getAddrInfo(address, info);
		//
		if (profiler::internal::enterEventCallback) {
			profiler::internal::enterEventCallback(info);
		}
	}
}

void PExit(void* address) {
	if (profiler::internal::enabled) {
		profiler::ProfilingAddrInfo info;
		profiler::internal::getAddrInfo(address, info);
		//
		if (profiler::internal::exitEventCallback) {
			profiler::internal::exitEventCallback(info);
		}
	}
}

void profiler::enable() {
	profiler::internal::enabled = true;
}

void profiler::disable() {
	profiler::internal::enabled = false;
}

void profiler::setEnterEventCallback(ProfilingEventCallback callback) {
	profiler::internal::enterEventCallback = callback;
}

void profiler::setExitEventCallback(ProfilingEventCallback callback) {
	profiler::internal::exitEventCallback = callback;
}

void profiler::setErrorCallback(ProfilingErrorCallback callback) {
	profiler::internal::errorCallback = callback;
}

void errorAsString(int errorCode, char* outbuffer, size_t outbuffersize) {
	profiler::internal::errorAsString(errorCode, outbuffer, outbuffersize);
}
