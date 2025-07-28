#pragma once

#ifndef IMGUIMANAGER_H
#define IMGUIMANAGER_H

#include <memory>
#include <vector>

#include "Common.h"

#include "ImGui/imgui.h"

class CameraManager;
class IPostProcess;
struct RenderStats;

class ImGuiManager
{
public:
	ImGuiManager();
	~ImGuiManager();

	static void RenderPostProcessWindow(double PipelineTime, std::vector<std::unique_ptr<IPostProcess>>& PostProcesses);
	static void RenderWorldHierarchyWindow();
	static void RenderCamerasWindow(std::shared_ptr<CameraManager>& CamManager);
	static void RenderStatsWindow(const RenderStats& Stats);
};

#endif
