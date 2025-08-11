#include <format>
#include <locale>

#include "ImGuiManager.h"
#include "Application.h"
#include "PostProcess/PostProcess.h"
#include "GameObject.h"
#include "Graphics.h"
#include "CameraManager.h"
#include "Profiler.h"
#include "Camera.h"

static int s_SelectedId = -1;

ImGuiManager::ImGuiManager()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
}

ImGuiManager::~ImGuiManager()
{
	ImGui::DestroyContext();
}

void ImGuiManager::RenderPostProcessWindow(double PipelineTime, std::vector<std::unique_ptr<IPostProcess>>& PostProcesses)
{
	Application* pApp = Application::GetSingletonPtr();
	
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	if (ImGui::Begin("Post Processes", nullptr, ImGuiWindowFlags_NoMove) && !PostProcesses.empty())
	{
		for (int i = 0; i < PostProcesses.size(); i++)
		{
			if (i != 0)
			{
				ImGui::Dummy(ImVec2(0.f, 10.f));
			}

			ImGui::PushID(i);
			ImGui::Checkbox("", &PostProcesses[i]->GetIsActive());
			ImGui::SameLine();
			if (ImGui::CollapsingHeader(PostProcesses[i]->GetName().c_str()))
			{
				PostProcesses[i]->RenderControls();
			}
			ImGui::PopID();
		}
	}

	ImGui::Dummy(ImVec2(0.f, 10.f));
	if (ImGui::Button("Reset ALL to defaults"))
	{
		for (const std::unique_ptr<IPostProcess>& pp : PostProcesses)
		{
			pp->ResetToDefaults();
			pp->ResetToInitialActive();
		}
	}

	ImGui::Text("Post process pipeline time: %.3f ms", PipelineTime);
	ImGui::End();
}

void ImGuiManager::RenderWorldHierarchyWindow()
{
	Application* pApp = Application::GetSingletonPtr();
	auto& GameObjects = pApp->GetGameObjects();
	
	ImGui::Begin("World Hierarchy", nullptr, ImGuiWindowFlags_NoMove);

	ImGui::BeginChild("##", ImVec2(0, 250), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
	for (int i = 0; i < GameObjects.size(); i++)
	{
		if (ImGui::Selectable(GameObjects[i]->GetName().c_str(), s_SelectedId == i))
		{
			s_SelectedId = i;
		}
	}
	ImGui::EndChild();

	ImGui::Dummy(ImVec2(0.f, 10.f));

	if (s_SelectedId >= 0)
	{
		GameObjects[s_SelectedId]->RenderControls();
		ImGui::Dummy(ImVec2(0.f, 5.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 10.f));

		for (auto& Comp : GameObjects[s_SelectedId]->GetComponents())
		{
			Comp->RenderControls();
			ImGui::Dummy(ImVec2(0.f, 5.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 10.f));
		}
	}

	ImGui::End();
}

void ImGuiManager::RenderCamerasWindow(std::shared_ptr<CameraManager>& CamManager)
{
	auto& Cameras = CamManager->GetCameras();
	auto& GameObjects = Application::GetSingletonPtr()->GetGameObjects();

	std::vector<const char*> CameraNames;
	for (const auto& Camera : Cameras)
	{
		CameraNames.push_back(Camera->GetName().c_str());
	}

	int ID = CamManager->GetActiveCameraID();

	ImGui::Begin("Cameras", nullptr, ImGuiWindowFlags_NoMove);

	if (ImGui::Combo("Active Camera", &ID, CameraNames.data(), (int)Cameras.size()))
	{
		CamManager->SetActiveCamera(ID);
	}

	if (ImGui::Button("Add New Camera"))
	{
		Cameras.emplace_back(std::make_shared<Camera>(Graphics::GetSingletonPtr()->GetDefaultProjMatrix()));
		GameObjects.push_back(Cameras.back());
		Cameras.back()->SetTransform(CamManager->GetActiveCamera()->GetTransform());
		CamManager->SetActiveCamera((int)Cameras.size() - 1);
	}

	if (ID != 0)
	{
		if (ImGui::Button("Delete Camera"))
		{
			auto c = CamManager->GetActiveCamera();

			Cameras.erase(Cameras.begin() + ID);

			auto it = std::find(GameObjects.begin(), GameObjects.end(), c);
			if (it != GameObjects.end())
			{
				GameObjects.erase(it);
			}

			CamManager->SetActiveCamera(0);
		}
	}

	ImGui::Dummy(ImVec2(0.f, 10.f));

	CamManager->GetActiveCamera()->RenderControls();

	ImGui::End();
}

void ImGuiManager::RenderStatsWindow(const RenderStats& Stats)
{
	ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_NoMove);

	ImGui::Text("Frame Time: %.3f ms/frame", Stats.FrameTime);
	ImGui::Text("FPS: %.1f", Stats.FPS);

	ImGui::Checkbox("Show Bounding Boxes", &Application::GetSingletonPtr()->GetShowBoundingBoxesRef());

	ImGui::Dummy(ImVec2(0.f, 10.f));

	ImGui::Text("Draw Calls: %s", std::format(std::locale("en_US.UTF-8"), "{:L}", Stats.DrawCalls).c_str());
	ImGui::Text("Compute Dispatches: %s", std::format(std::locale("en_US.UTF-8"), "{:L}", Stats.ComputeDispatches).c_str());

	ImGui::Dummy(ImVec2(0.f, 10.f));

	if (ImGui::CollapsingHeader("Triangles Rendered:", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const std::pair<std::string, UINT64>& Object : Stats.TrianglesRendered)
		{
			ImGui::Text("%s: %s", Object.first.c_str(), std::format(std::locale("en_US.UTF-8"), "{:L}", Object.second).c_str());
		}
	}

	if (ImGui::CollapsingHeader("Instances Rendered:", ImGuiTreeNodeFlags_DefaultOpen))
	{
		UINT64 TotalInstances = 0u;
		for (const std::pair<std::string, UINT64>& Object : Stats.InstancesRendered)
		{
			ImGui::Text("%s: %s", Object.first.c_str(), std::format(std::locale("en_US.UTF-8"), "{:L}", Object.second).c_str());
			TotalInstances += Object.second;
		}
		ImGui::Dummy(ImVec2(0.f, 10.f));
		ImGui::Text("Total: %s", std::format(std::locale("en_US.UTF-8"), "{:L}", TotalInstances).c_str());
	}

	ImGui::Dummy(ImVec2(0.f, 10.f));

	ImGui::End();
}
