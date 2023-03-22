#include "usagedisplay.hpp"
#include "../app.hpp"
#include "../utils/hwinfo.hpp"

#include <imgui.h>
#include <string>
#include <iostream>

UsageDisplay::UsageDisplay(App& app, float updatePerSec):
	_app(app)
{
	this->_timeFromLastUpdateSec = 0.0;
	this->_updateIntervalSec = 1.0 / updatePerSec;
	//
	this->_processId = 0;
	this->_runningCoreInd = 0;
	this->_fps = 0;
	this->_memUsagePercentage = 0;
	this->_memUsageMb = 0;
	this->_memAvailableMb = 0;
	this->_memPhysicalMb = 0;
	this->_cpuUsagePercentageAllCores = 0;
	this->_gpuUsagePercentage = 0;
	this->_gpuMemUsageMb = 0;
	this->_gpuMemAvailableMb = 0;
	this->_gpuMemPhysicalMb = 0;
	this->_coreCount = hwinfo::cpu::threadCount();
	this->_cpuUsagePercentagePerCore = new double[_coreCount];
	memset(this->_cpuUsagePercentagePerCore, 0, sizeof(double) * _coreCount);
}

void UsageDisplay::initialize() {

}

void UsageDisplay::ui(int w, int h) {
	//ImGui::ShowDemoWindow();
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoFocusOnAppearing;
	ImGui::Begin("Usage", nullptr, windowFlags);
	//
	ImGui::Text("PID: %-6d", this->_processId);
	ImGui::Text("Core: #%02d", this->_runningCoreInd);
	ImGui::Text("Fps: %-6.2f", this->_fps);
	//
	ImGui::SeparatorText("RAM");
	ImGui::Text("Used: %8.2f Mb", this->_memUsageMb);
	ImGui::Text("Free: %8.2f Gb", this->_memAvailableMb / 1024.0);
	ImGui::Text("Tot: %9.2f Gb", this->_memPhysicalMb / 1024.0);
	//
	ImGui::SeparatorText("GPU");
	ImGui::Text("Usage: %8.2f %%", this->_gpuUsagePercentage);
	ImGui::Text("Used: %8.2f Mb", this->_gpuMemUsageMb);
	ImGui::Text("Free: %8.2f Mb", this->_gpuMemAvailableMb);
	ImGui::Text("Tot: %9.2f Mb", this->_gpuMemPhysicalMb);
	//
	ImGui::SeparatorText("CPU");
	ImGui::Text("Main: %9.2f %%", this->_cpuUsagePercentagePerCore[this->_runningCoreInd]);
	ImGui::Text("All: %10.2f %%", this->_cpuUsagePercentageAllCores);
	const int ncores = this->_coreCount;
	const int ncols = (ncores > 8) ? 4 : 2;
	ImGui::BeginTable("cores", ncols);
	for (int core = 0; core < ncores; ++core) {
		float usage = (float)(this->_cpuUsagePercentagePerCore[core] / 100.0f);
		ImGui::TableNextColumn();
		//
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 size = ImGui::CalcTextSize("100%");
		size.x = ImGui::GetColumnWidth();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImU32 col = (usage < 0.01) ? IM_COL32(0, 255, 0, 128) : IM_COL32(255, 255 * (1.0 - usage), 0, 128);
		draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col);
		if (this->_runningCoreInd == core)
			draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32_WHITE, 0, 0, 2.0f);
		//
		ImGui::Text("%3.0f%%", this->_cpuUsagePercentagePerCore[core]);
	}
	ImGui::EndTable();
	ImGui::SetWindowPos(ImVec2(w - ImGui::GetWindowWidth(), 0), ImGuiCond_Always);
	ImGui::End();
}

void UsageDisplay::update(double dtSec) {
	this->_fpsAccumulator++;
	this->_timeFromLastUpdateSec += dtSec;
	if (this->_timeFromLastUpdateSec > this->_updateIntervalSec) {
		this->_timeFromLastUpdateSec -= this->_updateIntervalSec;
		//
		this->__update();
	}
}

void UsageDisplay::__update() {
	// Extra
	this->_processId = hwinfo::extra::pid();
	this->_runningCoreInd = hwinfo::extra::runningCoreInd();

	// Fps
	this->_fps = this->_fpsAccumulator / this->_updateIntervalSec;
	this->_fpsAccumulator = 0;

	// Memory
	this->_memUsageMb = hwinfo::mem::usageMb();
	this->_memAvailableMb = hwinfo::mem::availableMb();
	this->_memPhysicalMb = hwinfo::mem::physicalTotMb();
	this->_memUsagePercentage = (this->_memUsageMb / this->_memAvailableMb) * 100.0;

	// GPU
	this->_gpuUsagePercentage = hwinfo::gpu::usage();
	this->_gpuMemUsageMb = hwinfo::gpu::usageMb();
	this->_gpuMemAvailableMb = hwinfo::gpu::availableMb();
	this->_gpuMemPhysicalMb = hwinfo::gpu::physicalTotMb();

	// CPU
	this->_cpuUsagePercentageAllCores = hwinfo::cpu::usage(this->_cpuUsagePercentagePerCore);

}
