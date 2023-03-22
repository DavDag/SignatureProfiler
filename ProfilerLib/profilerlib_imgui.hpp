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
			constexpr float levelH = 25;
			constexpr int maxLevel = 8;
			float starty = ImGui::GetCursorPosY();

			// 1. Background Rect
			ImVec2 bgRectMin = ImVec2(0, starty);
			ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
			drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

			// 2. "Frame" Rect (for reference)
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size() - 1].time;
			DeltaNs frameDuration = ComputeDelta(frameBeg, frameEnd);
			drawList->AddRectFilled(
				ImVec2(0, starty + 0 * levelH),
				ImVec2(w, starty + 1 * levelH),
				IM_COL32(64, 64, 64, 255),
				0
			);

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
					int level = (int)stack.size() + 1;
					if (level <= maxLevel) {
						DeltaNs off = ComputeDelta(frameBeg, begEvent.time);
						DeltaNs dur = ComputeDelta(begEvent.time, ev.time);
						float beg = (off) / (float)frameDuration;
						float end = (off + dur) / (float)frameDuration;
						const auto& info = GetFuncInfo(begEvent.id);
						Crc32 hash = ComputeCRC32(info.funcName, strlen(info.funcName));
						ImU32 color = IM_COL32((hash & 0x000000ff), (hash & 0x0000ff00), (hash & 0x00ff0000), 255);
						ImVec2 posmin(beg * w, starty + (level + 0) * levelH);
						ImVec2 posmax(end * w, starty + (level + 1) * levelH);
						drawList->AddRectFilled(posmin, posmax, color, 0);
					}
				}
			}

			// 4. Selection Rect
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
				if (from >= 1) from = 1;
				if (from <= 0) from = 0;
				if (to >= 1) to = 1;
				if (to <= 0) to = 0;
			}
			if (from <= 0.001 && to >= 0.999) {
				// Do not draw selection since everything would be selected
			}
			else {
				ImVec2 selRectMin = ImVec2(w * from, starty);
				ImVec2 selRectMax = ImVec2(w * to, starty + levelH * maxLevel);
				drawList->AddRectFilled(selRectMin, selRectMax, IM_COL32(0, 0, 128, 64), 0);
			}

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + levelH * maxLevel);
		}

		///////////////////////////////////////////////////////////////
		ImGui::SeparatorText("");

		///////////////////////////////////////////////////////////////
		// Chart for 'selection'
		{
			constexpr float levelH = 50;
			constexpr int maxLevel = 12;
			float starty = ImGui::GetCursorPosY();

			// 0. Range
			float selFrom = (to > from) ? from : to;
			float selTo   = (to > from) ? to : from;

			// 1. Background Rect
			ImVec2 bgRectMin = ImVec2(0, starty);
			ImVec2 bgRectMax = ImVec2(w, starty + levelH * maxLevel);
			drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

			// 2. "Frame" Rect (for reference)
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size() - 1].time;
			DeltaNs frameDuration = ComputeDelta(frameBeg, frameEnd);
			drawList->AddRectFilled(
				ImVec2(0, starty + 0 * levelH),
				ImVec2(w, starty + 1 * levelH),
				IM_COL32(64, 64, 64, 255),
				0
			);
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
					int level = (int)stack.size() + 1;
					if (level <= maxLevel) {
						DeltaNs absOff = ComputeDelta(frameBeg, begEvent.time);
						DeltaNs relOff = absOff - zoomBegNs;
						DeltaNs absDur = ComputeDelta(begEvent.time, ev.time);
						DeltaNs relDur = absDur;
						if (relOff < 0) {
							relDur += relOff;
							relOff = 0;
						}
						if (relOff + relDur > zoomDuration) {
							relDur = zoomDuration - relOff;
						}
						float beg = (relOff) / (float)zoomDuration;
						float end = (relOff + relDur) / (float)zoomDuration + 0.001;
						const auto& info = GetFuncInfo(begEvent.id);
						Crc32 hash = ComputeCRC32(info.funcName, strlen(info.funcName));
						ImU32 color = IM_COL32((hash & 0x000000ff), (hash & 0x0000ff00), (hash & 0x00ff0000), 255);
						ImVec2 rectMin(beg * w, starty + (level + 0) * levelH);
						ImVec2 rectMax(end * w, starty + (level + 1) * levelH);
						drawList->AddRectFilled(rectMin, rectMax, color, 0);
						if (ImGui::IsMouseHoveringRect(rectMin, rectMax)) {
							ImGui::SetTooltip("%s", info.funcName);
							drawList->AddRect(rectMin, rectMax, IM_COL32(255, 255, 255, 255), 0);
						}
						ImVec2 textRectMin = ImVec2(rectMin.x + 2, rectMin.y + 2);
						ImVec2 textRectMax = ImVec2(rectMax.x - 2, rectMax.y - 2);
						ImGui::RenderTextClipped(textRectMin, textRectMax, info.funcName, nullptr, nullptr, ImVec2(0.5, 0.5));
					}
				}
			}

			// 4. Gestures
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
					//
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
				if (from >= 1) from = 1;
				if (from <= 0) from = 0;
				if (to >= 1) to = 1;
				if (to <= 0) to = 0;
			}
		}
	}
	ImGui::End();

	///////////////////////////////////////////////////////////////
	// Re-enable profiler (if needed)
	if (wasEnabled) profiler::Enable();
}
