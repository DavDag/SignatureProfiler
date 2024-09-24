#pragma once

#include "profilerlib.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <stack>

namespace profiler {
	void ImGuiRenderFrameHistory(
		const FrameHistory& history,
		int screenW, int screenH
	);

	namespace internal {
		bool __DrawFuncRect(
			profiler::FuncID func,
			DeltaUs funcOffset,
			DeltaUs funcDuration,
			DeltaUs timeFrameDuration,
			float totalW,
			float startY,
			int funcLevel,
			float levelHeight,
			bool showTooltip = false,
			bool showLabel = false,
			bool returnClicked = false
		);

		void __DrawTimeLines(
			int timeLinesMax,
			profiler::DeltaUs timeRounding,
			profiler::DeltaUs timeFrameDuration,
			profiler::DeltaUs timeFrameBeg,
			float totalW,
			float lineStartY,
			float lineHeight,
			const char* textFormat = "%2.1f(ms)",
			ImU32 lineColor = IM_COL32(128, 128, 128, 255)
		);
	}
}

///////////////////////////////////////////////////////////////////////////////

void profiler::ImGuiRenderFrameHistory(
	const FrameHistory& history,
	int screenW, int screenH
) {
	if (history.size() == 0) return;

	///////////////////////////////////////////////////////////////
	// Disable profiler
	auto wasEnabled = profiler::Disable();

	///////////////////////////////////////////////////////////////
	// State
	static int selectedHistoryIndexBeg = -1, selectedHistoryIndexEnd = -1;
	static float from = 0, to = 1; // selection range

	///////////////////////////////////////////////////////////////
	// Profiler Plot
	{
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_MenuBar;
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2((float)screenW, (float)screenH), ImGuiCond_Always);
		if (ImGui::Begin("Profiler", nullptr, windowFlags)) {
			///////////////////////////////////////////////////////////////
			// On appearing
			if (ImGui::IsWindowAppearing()) {
				ImGui::SetWindowFocus();
				selectedHistoryIndexBeg = -1;
				selectedHistoryIndexEnd = -1;
				from = 0;
				to = 1;
			}

			///////////////////////////////////////////////////////////////
			// Window's data
			float w = ImGui::GetWindowWidth();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGuiIO& io = ImGui::GetIO();
			static float chartLevelH = 50.f;
			static int chartMaxLevel = 12;
			static const char* statsTimeUnit[] = { "us", "ms", "s" };
			static float statsTimeUnitConv[] = { 1, 1'000, 1'000'000 };
			static int statsTimeUnitIndex = 0;

			///////////////////////////////////////////////////////////////
			// Header
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("Options")) {
					ImGui::SliderFloat("Plot event height", &chartLevelH, 5, 100, "%.0f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderInt("Plot max depth", &chartMaxLevel, 2, 32, "%d", ImGuiSliderFlags_AlwaysClamp);
					ImGui::Combo("Stats unit", &statsTimeUnitIndex, statsTimeUnit, 3);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Data")) {
					ImGui::Text("FrameEvent count: %d", history.size());
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			///////////////////////////////////////////////////////////////
			// Frame overview chart
			{
				constexpr float levelH = 10;
				constexpr int maxLevel = 8;
				float starty = ImGui::GetCursorPosY() - ImGui::GetScrollY() + 24;

				// 1. Background Rect
				ImVec2 bgRectMin = ImVec2(0, starty);
				ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
				drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

				// 2. Frame vars
				TimeStamp frameBeg = history[0].time;
				TimeStamp frameEnd = history[history.size() - 1].time;
				DeltaUs frameDuration = ComputeDelta(frameBeg, frameEnd);

				// 3. Events Rects
				static std::stack<profiler::FrameHistoryEntry> stack{}; // exploration stack
				while (!stack.empty()) stack.pop();
				for (const auto& ev : history) {
					if (ev.id != EmptyFuncID) {
						stack.push(ev);
					}
					else {
						const auto& begEvent = stack.top();
						stack.pop();
						int level = (int)stack.size();
						if (level < maxLevel) {
							DeltaUs off = ComputeDelta(frameBeg, begEvent.time);
							DeltaUs dur = ComputeDelta(begEvent.time, ev.time);
							internal::__DrawFuncRect(
								begEvent.id, off, dur, frameDuration,
								w, starty, level, levelH,
								false, false, false
							);
						}
					}
				}

				// 4. Time lines
				internal::__DrawTimeLines(6, 1000, frameDuration, 0, w, starty, maxLevel * levelH);

				// 5. Selection Rect
				static float oldX = 0; // selection drag origin x
				if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(bgRectMin, bgRectMax)) {
					io.WantCaptureMouse = true;
					// Single click: setup for drag
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						oldX = ImGui::GetMousePos().x;
						from = oldX / w;
						from = (from < 0) ? 0 : from;
					}
					// Double click: reset selection (0,1)
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						from = 0;
						to = 1;
					}
					// Drag: update selection
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
						float deltaX = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
						to = (oldX + deltaX) / w;
						// Clamp when close to edge
						if (to <= 0.02) to = 0;
						if (to >= (1 - 0.01)) to = 1;
					}
					// Wheel: move selection
					float wheel = io.MouseWheel * (io.KeyShift ? 1 : 0);
					if (wheel != 0) {
						from += 0.025f * wheel;
						to += 0.025f * wheel;
					}
					// Clamp values
					from = std::min(from, 1.f);
					from = std::max(from, 0.f);
					to = std::min(to, 1.f);
					to = std::max(to, 0.f);
				}
				if (from <= 0.001 && to >= 0.999) {
					// Do not draw selection since everything would be selected
				}
				else {
					ImVec2 selRectMin = ImVec2(w * from, starty);
					ImVec2 selRectMax = ImVec2(w * to, starty + levelH * maxLevel);
					drawList->AddRectFilled(selRectMin, selRectMax, IM_COL32(0, 0, 128, 64), 0);
				}

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + levelH * maxLevel + 24);
			}

			///////////////////////////////////////////////////////////////
			ImGui::Spacing();

			///////////////////////////////////////////////////////////////
			// Chart for 'selection'
			{
				float levelH = chartLevelH;
				int maxLevel = chartMaxLevel;
				float starty = ImGui::GetCursorPosY() - ImGui::GetScrollY() + 24;

				// 0. Range
				float selFrom = (to > from) ? from : to;
				float selTo = (to > from) ? to : from;

				// 1. Background Rect
				ImVec2 bgRectMin = ImVec2(0, starty);
				ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
				drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

				// 2. Frame vars (zoomed)
				TimeStamp frameBeg = history[0].time;
				TimeStamp frameEnd = history[history.size() - 1].time;
				DeltaUs frameDuration = ComputeDelta(frameBeg, frameEnd);
				DeltaUs zoomBegUs = (DeltaUs)(frameDuration * selFrom);
				DeltaUs zoomEndUs = (DeltaUs)(frameDuration * selTo);
				DeltaUs zoomDuration = zoomEndUs - zoomBegUs;

				// 3. Events Rects
				static std::stack<int> stack{}; // exploration stack
				while (!stack.empty()) stack.pop();
				for (int i = 0; i < history.size(); ++i) {
					const auto& ev = history[i];
					if (ev.id != EmptyFuncID) {
						stack.push(i);
					}
					else {
						int begEventIndex = stack.top(); stack.pop();
						const auto& begEvent = history[begEventIndex];
						int level = (int)stack.size();
						if (level < maxLevel) {
							DeltaUs absOff = ComputeDelta(frameBeg, begEvent.time);
							DeltaUs absDur = ComputeDelta(begEvent.time, ev.time);
							DeltaUs relOff = absOff - zoomBegUs;
							DeltaUs relDur = absDur;
							if (relOff < 0) {
								relDur += relOff;
								relOff = 0;
							}
							if (relOff + relDur > zoomDuration) {
								relDur = zoomDuration - relOff;
							}
							bool clicked = internal::__DrawFuncRect(
								begEvent.id, relOff, relDur, zoomDuration,
								w, starty, level, levelH,
								true, true, true
							);
							if (clicked) {
								selectedHistoryIndexBeg = begEventIndex;
								selectedHistoryIndexEnd = i;
							}
						}
					}
				}

				// 4. Time lines
				internal::__DrawTimeLines(4, 100, zoomDuration, zoomBegUs, w, starty, maxLevel * levelH);

				// 5. Gestures
				static float oldX = 0; // drag origin x
				if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(bgRectMin, bgRectMax)) {
					io.WantCaptureMouse = true;
					// Single click: setup for drag
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						oldX = ImGui::GetMousePos().x;
					}
					// Drag: move selection
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
						float deltaX = -ImGui::GetMouseDragDelta().x;
						ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
						oldX = deltaX;
						float delta = (deltaX / w) * (selTo - selFrom);
						from += delta;
						to += delta;
					}
					// Wheel: zoom at mouse coords
					float wheel = io.MouseWheel * (io.KeyShift ? 1 : 0);
					if (wheel != 0) {
						float bias = ImGui::GetMousePos().x / w;
						float center = bias * (selTo - selFrom) + selFrom;
						from += (center - from) * 0.2f * wheel;
						to += (center - to) * 0.2f * wheel;
					}
					// Clamp values
					from = std::min(from, 1.f);
					from = std::max(from, 0.f);
					to = std::min(to, 1.f);
					to = std::max(to, 0.f);
				}

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + levelH * maxLevel + 24);
			}

			///////////////////////////////////////////////////////////////
			// Selected Func Info & Stats
			{
				if (selectedHistoryIndexBeg != -1 && selectedHistoryIndexEnd != -1) {
					///////////////////////////////////////////////////////////////
					// Data
					const char* timeUnit = statsTimeUnit[statsTimeUnitIndex];
					float timeUnitConvFromUs = statsTimeUnitConv[statsTimeUnitIndex];
					FuncID funcID = history[selectedHistoryIndexBeg].id;
					const auto& funcInfo = GetFuncInfo(funcID);
					const auto& funcStats = GetFuncStats(funcID);
					TimeStamp funcBeg = history[selectedHistoryIndexBeg].time;
					TimeStamp funcEnd = history[selectedHistoryIndexEnd].time;
					DeltaUs funcDur = ComputeDelta(funcBeg, funcEnd);
					TimeStamp frameBeg = history[0].time;
					TimeStamp frameEnd = history[history.size() - 1].time;
					DeltaUs frameDur = ComputeDelta(frameBeg, frameEnd);
					DeltaUs funcBegRel = ComputeDelta(frameBeg, funcBeg);
					DeltaUs funcEndRel = ComputeDelta(frameBeg, funcEnd);

					///////////////////////////////////////////////////////////////
					// Display data
					ImGui::SeparatorText("Selection");
					if (ImGui::BeginTable("Table", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
						///////////////////////////
						ImGui::TableSetupColumn("Source Code");
						ImGui::TableSetupColumn("Performance");
						ImGui::TableSetupColumn("Current Frame");
						ImGui::TableSetupColumn("Stack Trace");
						ImGui::TableHeadersRow();
						ImGui::TableNextRow();

						///////////////////////////
						// Source Code
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%s", funcInfo.fileName);
						ImGui::Text("%s", funcInfo.funcNameExt);
						ImGui::Text("%s.%d", funcInfo.funcName, funcInfo.fileLine);

						///////////////////////////
						// Performance
						ImGui::TableSetColumnIndex(1);
						ImGui::Text("Max: %14.3f (%s)", funcStats.usMax / timeUnitConvFromUs, timeUnit);
						ImGui::Text("Min: %14.3f (%s)", funcStats.usMin / timeUnitConvFromUs, timeUnit);
						ImGui::Text("Avg: %14.3f (%s)", funcStats.usAvg / timeUnitConvFromUs, timeUnit);
						ImGui::Text("Tot: %14.3f (%s)", funcStats.usTot / timeUnitConvFromUs, timeUnit);
						ImGui::Text("#Calls: %12lld", funcStats.invocationCount);

						///////////////////////////
						// Current Frame
						ImGui::TableSetColumnIndex(2);
						ImGui::Text("Beg: %14.3f (%s)", funcBegRel / timeUnitConvFromUs, timeUnit);
						ImGui::Text("End: %14.3f (%s)", funcEndRel / timeUnitConvFromUs, timeUnit);
						ImGui::Text("Dur: %14.3f (%s)", funcDur / timeUnitConvFromUs, timeUnit);

						///////////////////////////
						// Stack Trace
						ImGui::TableSetColumnIndex(3);
						static std::vector<int> funcStack{};
						funcStack.clear();
						int endcount = 0;
						for (int i = selectedHistoryIndexBeg - 1; i >= 0; --i) {
							const auto& ev = history[i];
							if (ev.id == EmptyFuncID) {
								endcount++;
							}
							else {
								if (endcount > 0) {
									endcount--;
								}
								else {
									funcStack.push_back(i);
								}
							}
						}
						for (int i = (int)funcStack.size() - 1; i >= 0; --i) {
							const auto& ev = history[funcStack[i]];
							const auto& stackFuncInfo = GetFuncInfo(ev.id);
							ImGui::Text(">> %s.%d", stackFuncInfo.funcName, stackFuncInfo.fileLine);
						}
						ImGui::Text(">> %s.%d", funcInfo.funcName, funcInfo.fileLine);

						///////////////////////////
						ImGui::EndTable();
					}
				}
			}
		}
		ImGui::End();
	}

	///////////////////////////////////////////////////////////////
	// Re-enable profiler (if needed)
	if (wasEnabled) profiler::Enable();
}

bool profiler::internal::__DrawFuncRect(
	profiler::FuncID func,
	DeltaUs funcOffset,
	DeltaUs funcDuration,
	DeltaUs timeFrameDuration,
	float totalW,
	float startY,
	int funcLevel,
	float levelHeight,
	bool showTooltip /*= false*/,
	bool showLabel /*= false*/,
	bool returnClicked /*= false*/
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float beg = (funcOffset) / (float)timeFrameDuration;
	float end = (funcOffset + funcDuration) / (float)timeFrameDuration;
	end = std::max(end, 0.001f);
	const auto& info = profiler::GetFuncInfo(func);
	CRC32 hash = ComputeCRC32(info.funcName, (int)strlen(info.funcName));
	ImU32 color = IM_COL32((hash & 0x000000ff), (hash & 0x0000ff00), (hash & 0x00ff0000), 255);
	ImVec2 posmin(beg * totalW, startY + (funcLevel + 0) * levelHeight);
	ImVec2 posmax(end * totalW, startY + (funcLevel + 1) * levelHeight);
	drawList->AddRectFilled(posmin, posmax, color, 0);
	if (showTooltip) {
		if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(posmin, posmax)) {
			ImGui::SetTooltip("%s", info.funcName);
			drawList->AddRect(posmin, posmax, IM_COL32(255, 255, 255, 255), 0);
		}
	}
	if (showLabel) {
		ImVec2 textmin = ImVec2(posmin.x + 2, posmin.y + 2);
		ImVec2 textmax = ImVec2(posmax.x - 2, posmax.y - 2);
		ImGui::RenderTextClipped(textmin, textmax, info.funcName, nullptr, nullptr, ImVec2(0.5, 0.5));
	}
	if (returnClicked) {
		if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(posmin, posmax)) {
			return ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		}
	}
	return false;
}

void profiler::internal::__DrawTimeLines(
	int timeLinesMax,
	profiler::DeltaUs timeRounding,
	profiler::DeltaUs timeFrameDuration,
	profiler::DeltaUs timeFrameBeg,
	float totalW,
	float lineStartY,
	float lineHeight,
	const char* textFormat /*= "%2.1f(ms)"*/,
	ImU32 lineColor /*= IM_COL32(128, 128, 128, 255)*/
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	static char buff[64] = { {'\0'} };
	DeltaUs timeLinesCount = (timeFrameDuration / timeRounding) + 1;
	profiler::DeltaUs timeRoundingRel = timeRounding * ((timeLinesCount / timeLinesMax) + 1);
	profiler::DeltaUs timeFrameBegRel = timeFrameBeg - (timeFrameBeg % timeRoundingRel);
	for (int i = 0; i < timeLinesCount; ++i) {
		profiler::DeltaUs timeUs = timeFrameBegRel + timeRoundingRel * (i + 1LL);
		float timePercentage = (timeUs - timeFrameBeg) / (float)timeFrameDuration;
		ImVec2 lineMin = ImVec2(timePercentage * totalW, lineStartY);
		ImVec2 lineMax = ImVec2(timePercentage * totalW, lineStartY + lineHeight);
		drawList->AddLine(lineMin, lineMax, lineColor);
		int n = sprintf_s(buff, textFormat, timeUs / 1'000.f);
		ImVec2 textSize = ImGui::CalcTextSize(buff, buff + n);
		ImVec2 textPos = ImVec2(lineMin.x - textSize.x / 2, lineMin.y - textSize.y);
		ImGui::RenderText(textPos, buff, buff + n);
	}
}
