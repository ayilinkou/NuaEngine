#pragma once

#ifndef IMGUIMANAGER_H
#define IMGUIMANAGER_H

#include <memory>

#include "Common.h"

#include "ImGui/imgui.h"

class CameraManager;
struct RenderStats;

class ImGuiManager
{
public:
	ImGuiManager();
	~ImGuiManager();

	static void RenderPostProcessWindow(double PipelineTime);
	static void RenderWorldHierarchyWindow();
	static void RenderCamerasWindow(std::shared_ptr<CameraManager>& CamManager);
	static void RenderStatsWindow(const RenderStats& Stats);
};

#endif
