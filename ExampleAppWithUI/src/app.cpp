#include "app.hpp"
#include "./utils/hwinfo.hpp"
#include "./utils/opengl/opengl.hpp"

#include "../../ProfilerLib/profilerlib.hpp"
#include "../../ProfilerLib/profilerlib_imgui.hpp"

#include <GLFW/glfw3.h>

App::App():
	_display(*this, 1)
{
	this->_showProfilerUI = false;
}

void App::initialize() {
	this->_display.initialize();
}

void App::update(double dtSec) {
	if (this->isFrozen()) return;
	//
	this->_display.update(dtSec);
}

void App::render(int w, int h) {
	GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
	this->_display.ui(w, h);
	if (this->_showProfilerUI) {
		// TODO: ISSUE 1st time
		profiler::ImGuiRenderFrameHistory(
			profiler::GetFrameHistory(),
			w, h
		);
	}
}

void App::onKeyDown(int key) {
	if (key == GLFW_KEY_TAB)
		this->_showProfilerUI = !this->_showProfilerUI;
}

void App::onKeyUp(int key) {

}

void App::onMouseBtnDown(int btn) {

}

void App::onMouseBtnUp(int btn) {

}

void App::onMouseWheel(double dx, double dy) {

}

void App::onResize(int width, int height) {

}

void App::raw(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	this->__log(LogType::NONE, IM_COL32(255, 255, 255, 255), fmt, args);
	va_end(args);
}

void App::inf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	this->__log(LogType::INFO, IM_COL32(192, 192, 192, 255), fmt, args);
	va_end(args);
}

void App::deb(const char* fmt, ...) {
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	this->__log(LogType::DEBUG, IM_COL32(128, 128, 128, 255), fmt, args);
	va_end(args);
#endif
}

void App::err(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	this->__log(LogType::ERROR, IM_COL32(255, 64, 64, 255), fmt, args);
	va_end(args);
}

void App::__log(LogType type, ImU32 col, const char* fmt, va_list args) {
	static const char* prefs[] = { "raw", "info", "error", "debug" };
	static char buff[4096] = { {'\0'} };
	vsprintf_s(buff, fmt, args);
	fprintf(stdout, "[%-8.8s]: %s\n", prefs[type], buff);
}

bool App::isFrozen() const {
	return this->_showProfilerUI;
}
