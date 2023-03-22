#pragma once

#include "./ui/usagedisplay.hpp"

#include <imgui.h>
#include <vector>

enum LogType {
	NONE = 0,  // raw
	INFO = 1,  // information
	ERROR = 2, // errors
	DEBUG = 3, // extra / debug-only
};

class App {
public:
	App();
	//
	void initialize();
	void render(int w, int h);
	void update(double dtSec);
	//
	void onKeyDown(int key);
	void onKeyUp(int key);
	void onMouseBtnDown(int btn);
	void onMouseBtnUp(int btn);
	void onMouseWheel(double dx, double dy);
	void onResize(int w, int h);
	//
	void raw(const char* fmt, ...);
	void inf(const char* fmt, ...);
	void deb(const char* fmt, ...);
	void err(const char* fmt, ...);
	//
	bool isFrozen() const;

private:
	void __log(LogType type, ImU32 col, const char* fmt, va_list args);

private:
	bool _showProfilerUI;
	UsageDisplay _display;
};

