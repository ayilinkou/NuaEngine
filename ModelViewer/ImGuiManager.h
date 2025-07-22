#pragma once

#ifndef IMGUIMANAGER_H
#define IMGUIMANAGER_H

#include "Common.h"

#include "ImGui/imgui.h"

class ImGuiManager
{
public:
	ImGuiManager();
	~ImGuiManager();

	static void RenderPostProcessWindow(double PipelineTime);
	static void RenderWorldHierarchyWindow();
	static void RenderCamerasWindow();
	static void RenderStatsWindow(const RenderStats& Stats);
};

#endif
