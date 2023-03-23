#pragma once

#include "profilerlib.hpp"
#include "crc32.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <stack>

namespace profiler {
	void ImGuiRenderFrameHistory(
		const std::vector<FrameHistoryEntry>& history,
		int screenW, int screenH
	);

	namespace internal {
		void __DrawFuncRect(
			profiler::FuncID func,
			DeltaNs funcOffset,
			DeltaNs funcDuration,
			DeltaNs timeFrameDuration,
			float totalW,
			float startY,
			int funcLevel,
			float levelHeight,
			bool showTooltip = false,
			bool showLabel = false
		);
		
		void __DrawTimeLines(
			int timeLinesMax,
			profiler::DeltaNs timeRounding,
			profiler::DeltaNs timeFrameDuration,
			profiler::DeltaNs timeFrameBeg,
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
	const std::vector<FrameHistoryEntry>& history,
	int screenW, int screenH
) {
	///////////////////////////////////////////////////////////////
	// Disable profiler
	auto wasEnabled = profiler::Disable();

	///////////////////////////////////////////////////////////////
	// ImGui Window
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav;
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)screenW, (float)screenH), ImGuiCond_Always);
	if (ImGui::Begin("Profiler", nullptr, windowFlags)) {
		///////////////////////////////////////////////////////////////
		// 0. Variables
		static float from = 0, to = 1; // selection range
		if (ImGui::IsWindowAppearing()) {
			from = 0;
			to = 1;
		}

		///////////////////////////////////////////////////////////////
		// Window's data
		float w = ImGui::GetWindowWidth();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		///////////////////////////////////////////////////////////////
		// Header
		ImGui::Text("FrameEvent count: %d", history.size());

		///////////////////////////////////////////////////////////////
		// Frame overview chart
		{
			constexpr float levelH = 10;
			constexpr int maxLevel = 8;
			float starty = ImGui::GetCursorPosY() + 24;

			// 1. Background Rect
			ImVec2 bgRectMin = ImVec2(0, starty);
			ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
			drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

			// 2. Frame vars
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size() - 1].time;
			DeltaNs frameDuration = ComputeDelta(frameBeg, frameEnd);

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
					if (level <= maxLevel) {
						DeltaNs off = ComputeDelta(frameBeg, begEvent.time);
						DeltaNs dur = ComputeDelta(begEvent.time, ev.time);
						internal::__DrawFuncRect(begEvent.id, off, dur, frameDuration, w, starty, level, levelH, false, false);
					}
				}
			}

			// 4. Time lines
			internal::__DrawTimeLines(6, 1000, frameDuration, 0, w, starty, maxLevel * levelH);

			// 5. Selection Rect
			static float oldX = 0; // selection drag origin x
			if (ImGui::IsMouseHoveringRect(bgRectMin, bgRectMax)) {
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
				ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
				if (float wheel = ImGui::GetIO().MouseWheel; wheel != 0) {
					from += 0.0025 * wheel;
					to   += 0.0025 * wheel;
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

			ImGui::SetCursorPosY(starty + levelH * maxLevel);
		}

		///////////////////////////////////////////////////////////////
		ImGui::Spacing();

		///////////////////////////////////////////////////////////////
		// Chart for 'selection'
		{
			constexpr float levelH = 50;
			constexpr int maxLevel = 12;
			float starty = ImGui::GetCursorPosY() + 24;

			// 0. Range
			float selFrom = (to > from) ? from : to;
			float selTo   = (to > from) ? to : from;

			// 1. Background Rect
			ImVec2 bgRectMin = ImVec2(0, starty);
			ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
			drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

			// 2. Frame vars (zoomed)
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size() - 1].time;
			DeltaNs frameDuration = ComputeDelta(frameBeg, frameEnd);
			DeltaNs zoomBegNs = frameDuration * selFrom;
			DeltaNs zoomEndNs = frameDuration * selTo;
			DeltaNs zoomDuration = zoomEndNs - zoomBegNs;

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
					if (level <= maxLevel) {
						DeltaNs absOff = ComputeDelta(frameBeg, begEvent.time);
						DeltaNs absDur = ComputeDelta(begEvent.time, ev.time);
						DeltaNs relOff = absOff - zoomBegNs;
						DeltaNs relDur = absDur;
						if (relOff < 0) {
							relDur += relOff;
							relOff = 0;
						}
						if (relOff + relDur > zoomDuration) {
							relDur = zoomDuration - relOff;
						}
						internal::__DrawFuncRect(begEvent.id, relOff, relDur, zoomDuration, w, starty, level, levelH, true, true);
					}
				}
			}

			// 4. Time lines
			internal::__DrawTimeLines(4, 100, zoomDuration, zoomBegNs, w, starty, maxLevel * levelH);

			// 5. Gestures
			static float oldX = 0; // drag origin x
			if (ImGui::IsMouseHoveringRect(bgRectMin, bgRectMax)) {
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
				if (float wheel = ImGui::GetIO().MouseWheel; wheel != 0) {
					float bias = ImGui::GetMousePos().x / w;
					float center = bias * (selTo - selFrom) + selFrom;
					from += (center - from) * 0.25 * wheel;
					to   += (center - to  ) * 0.25 * wheel;
				}
				// Clamp values
				from = std::min(from, 1.f);
				from = std::max(from, 0.f);
				to = std::min(to, 1.f);
				to = std::max(to, 0.f);
			}

			ImGui::SetCursorPosY(starty + levelH * maxLevel);
		}
	}
	ImGui::End();

	///////////////////////////////////////////////////////////////
	// Re-enable profiler (if needed)
	if (wasEnabled) profiler::Enable();
}

void profiler::internal::__DrawFuncRect(
	profiler::FuncID func,
	DeltaNs funcOffset,
	DeltaNs funcDuration,
	DeltaNs timeFrameDuration,
	float totalW,
	float startY,
	int funcLevel,
	float levelHeight,
	bool showTooltip /*= false*/,
	bool showLabel /*= false*/
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float beg = (funcOffset               ) / (float)timeFrameDuration;
	float end = (funcOffset + funcDuration) / (float)timeFrameDuration;
	end = std::max(end, 0.001f);
	const auto& info = profiler::GetFuncInfo(func);
	Crc32 hash = ComputeCRC32(info.funcName, strlen(info.funcName));
	ImU32 color = IM_COL32((hash & 0x000000ff), (hash & 0x0000ff00), (hash & 0x00ff0000), 255);
	ImVec2 posmin(beg * totalW, startY + (funcLevel + 0) * levelHeight);
	ImVec2 posmax(end * totalW, startY + (funcLevel + 1) * levelHeight);
	// Rect
	drawList->AddRectFilled(posmin, posmax, color, 0);
	// Tooltip
	if (showTooltip) {
		if (ImGui::IsMouseHoveringRect(posmin, posmax)) {
			ImGui::SetTooltip("%s", info.funcName);
			drawList->AddRect(posmin, posmax, IM_COL32(255, 255, 255, 255), 0);
		}
	}
	// Label
	if (showLabel) {
		ImVec2 textmin = ImVec2(posmin.x + 2, posmin.y + 2);
		ImVec2 textmax = ImVec2(posmax.x - 2, posmax.y - 2);
		ImGui::RenderTextClipped(textmin, textmax, info.funcName, nullptr, nullptr, ImVec2(0.5, 0.5));
	}
}

void profiler::internal::__DrawTimeLines(
	int timeLinesMax,
	profiler::DeltaNs timeRounding,
	profiler::DeltaNs timeFrameDuration,
	profiler::DeltaNs timeFrameBeg,
	float totalW,
	float lineStartY,
	float lineHeight,
	const char* textFormat /*= "%2.1f(ms)"*/,
	ImU32 lineColor /*= IM_COL32(128, 128, 128, 255)*/
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	static char buff[64] = { {'\0'} };
	int timeLinesCount = (timeFrameDuration / timeRounding) + 1;
	profiler::DeltaNs timeRoundingRel = timeRounding * ((timeLinesCount / timeLinesMax) + 1);
	profiler::DeltaNs timeFrameBegRel = timeFrameBeg - (timeFrameBeg % timeRoundingRel);
	for (int i = 0; i < timeLinesCount; ++i) {
		profiler::DeltaNs timeNs = timeFrameBegRel + timeRoundingRel * (i + 1);
		float timePercentage = (timeNs - timeFrameBeg) / (float)timeFrameDuration;
		// Line
		ImVec2 lineMin = ImVec2(timePercentage * totalW, lineStartY);
		ImVec2 lineMax = ImVec2(timePercentage * totalW, lineStartY + lineHeight);
		drawList->AddLine(lineMin, lineMax, lineColor);
		// Text
		int n = sprintf_s(buff, textFormat, timeNs / 1'000.f);
		ImVec2 textSize = ImGui::CalcTextSize(buff, buff + n);
		ImVec2 textPos = ImVec2(lineMin.x - textSize.x / 2, lineMin.y - textSize.y);
		ImGui::RenderText(textPos, buff, buff + n);
	}
}
