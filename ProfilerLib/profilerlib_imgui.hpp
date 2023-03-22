#pragma once

#include "profilerlib.hpp"
#include "crc32.hpp"
#include <imgui.h>
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
	ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)screenW-32, (float)screenH-32), ImGuiCond_Always);
	if (ImGui::Begin("Profiler", nullptr, windowFlags)) {
		///////////////////////////////////////////////////////////////
		// 0. Variables
		static float from = 0, to = 1; // selection range
		constexpr float LEVEL_H = 25;  // event height
		constexpr int MAX_LEVEL = 8;   // max level
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
			// 1. Background Rect
			ImVec2 bgRectMin = ImVec2(0, ImGui::GetCursorPosY());
			ImVec2 bgRectMax = ImVec2(w, ImGui::GetCursorPosY() + LEVEL_H * MAX_LEVEL);
			drawList->AddRectFilled(bgRectMin, bgRectMax, IM_COL32(128, 128, 128, 64), 0);

			// 2. "Frame" Rect (for reference)
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size() - 1].time;
			DeltaNs frameDuration = ComputeDelta(frameBeg, frameEnd);
			drawList->AddRectFilled(
				ImVec2(0, ImGui::GetCursorPosY() + 0 * LEVEL_H),
				ImVec2(w, ImGui::GetCursorPosY() + 1 * LEVEL_H),
				IM_COL32(64, 64, 64, 255),
				0
			);

			// 3. Events rects
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
					if (level <= MAX_LEVEL) {
						DeltaNs off = ComputeDelta(frameBeg, begEvent.time);
						DeltaNs dur = ComputeDelta(begEvent.time, ev.time);
						float beg = (off) / (float)frameDuration;
						float end = (off + dur) / (float)frameDuration;
						const auto& info = GetFuncInfo(begEvent.id);
						Crc32 hash = ComputeCRC32(info.funcName, strlen(info.funcName));
						drawList->AddRectFilled(
							ImVec2(beg * w, ImGui::GetCursorPosY() + (level + 0) * LEVEL_H),
							ImVec2(end * w, ImGui::GetCursorPosY() + (level + 1) * LEVEL_H),
							IM_COL32((hash & 0x000000ff), (hash & 0x0000ff00), (hash & 0x00ff0000), 255),
							0
						);
					}
				}
			}

			// 4. Selection Rect
			static float oldX = 0; // selection drag origin x
			if (ImGui::IsMouseHoveringRect(bgRectMin, bgRectMax)) {
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					oldX = ImGui::GetMousePos().x;
					from = oldX / w;
					from = (from < 0) ? 0 : from;
				}
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					from = 0;
					to = 1;
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					float deltaX = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
					to = (oldX + deltaX) / w;
				}
				if (to >= -0.02 && to <= 0.02) to = 0;
				if (to <= -(1-0.01) || to >= (1-0.01)) to = (to < 0) ? -1 : 1;
			}
			if (from <= 0.001 && to >= 0.999) {
				// Do not draw selection since everything would be selected
			}
			else {
				ImVec2 selRectMin = ImVec2(w * from, ImGui::GetCursorPosY());
				ImVec2 selRectMax = ImVec2(w * to, ImGui::GetCursorPosY() + LEVEL_H * MAX_LEVEL);
				drawList->AddRectFilled(selRectMin, selRectMax, IM_COL32(0, 0, 128, 64), 0);
			}
		}

		//
		if (false) {
			std::vector<TimeStamp> endings(history.size());
			std::stack<int> stack;
			for (int i = 0; i < history.size(); i++) {
				const auto& ev = history[i];
				if (ev.id == EmptyFuncID) {
					endings[stack.top()] = ev.time;
					stack.pop();
				}
				else {
					stack.push(i);
				}
			}
			TimeStamp frameBeg = history[0].time;
			TimeStamp frameEnd = history[history.size()-1].time;
			DeltaNs frameDelta = ComputeDelta(frameBeg, frameEnd);
			ImGui::Text("%*.s%-*.*s: %8lld (ns)", 0, "", 32, 32, "Frame", frameDelta);
			int level = 1;
			for (int i = 0; i < history.size(); i++) {
				const auto& ev = history[i];
				if (ev.id == EmptyFuncID) {
					--level;
				}
				else {
					const auto& info = GetFuncInfo(ev.id);
					DeltaNs delta = ComputeDelta(ev.time, endings[i]);
					DeltaNs fromBeg = ComputeDelta(frameBeg, ev.time);
					ImGui::Text("%*.s%-*.*s: %8lld (ns)", level, "", 32-level, 32-level, info.funcName, delta);
					++level;
				}
			}
		}
	}
	ImGui::End();

	///////////////////////////////////////////////////////////////
	// Re-enable profiler (if needed)
	if (wasEnabled) profiler::Enable();
}
