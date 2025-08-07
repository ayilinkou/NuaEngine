#pragma once

#ifndef APPLICATION_H
#define APPLICATION_H

#include <chrono>
#include <vector>
#include <memory>

#include "wrl.h"

#include "Graphics.h"
#include "Common.h"

constexpr bool FULL_SCREEN = false;
constexpr bool VSYNC_ENABLED = false;
constexpr float SCREEN_FAR = 2000.f;
constexpr float SCREEN_NEAR = 0.1f;

class InstancedShader;
class Model;
class ModelData;
class Light;
class Camera;
class GameObject;
class Skybox;
class Landscape;
class FrustumRenderer;
class BoxRenderer;
class FrustumCuller;
class CameraManager;
class Profiler;
class PostProcessManager;

class Application
{
private:
	static Application* m_Instance;

	Application();

public:
	static Application* GetSingletonPtr()
	{
		if (!m_Instance)
		{
			m_Instance = new Application();
		}
		return m_Instance;
	}
	
	bool Initialise(int ScreenWidth, int ScreenHeight, HWND hWnd);
	void Shutdown();
	bool Tick();

	HWND GetWindowHandle() const { return m_hWnd; }
	Graphics* GetGraphics() const { return m_Graphics; }

	InstancedShader* GetInstancedShader() const { return m_InstancedShader.get(); }
	std::shared_ptr<FrustumCuller> GetFrustumCuller() const { return m_FrustumCuller; }
	std::shared_ptr<PostProcessManager> GetPostProcessManager() const { return m_PostProcessManager; }
	std::shared_ptr<BoxRenderer> GetBoxRenderer() const { return m_BoxRenderer; }
	std::shared_ptr<Profiler> GetProfiler() const { return m_Profiler; }

	std::vector<std::shared_ptr<GameObject>>& GetGameObjects() { return m_GameObjects; }

	double GetDeltaTime() const { return m_DeltaTime; }
	double GetAppTime() const { return m_AppTime; }
	uint32_t GetFrameIndex() const { return m_FrameIndex; }
	bool& GetShowBoundingBoxesRef() { return m_bShowBoundingBoxes; }

private:
	bool Render();
	bool RenderScene();
	void RenderModels();
	//bool RenderTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureView);

	// Depending on how many windows are open, this can actually have a significant cost to frame times
	void RenderImGui();

	void ApplyPostProcesses(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> CurrentRTV, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SecondaryRTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CurrentSRV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SecondarySRV, bool& DrawingForward);

	void UpdateGlobalConstantBuffer();

	void ProcessInput();
	void ToggleShowCursor();

private:	
	HWND m_hWnd;

	Graphics* m_Graphics;

	std::shared_ptr<PostProcessManager> m_PostProcessManager;
	std::shared_ptr<CameraManager> m_CameraManager;
	std::shared_ptr<Profiler> m_Profiler;

	std::unique_ptr<InstancedShader> m_InstancedShader;
	std::unique_ptr<Skybox> m_Skybox;
	std::shared_ptr<BoxRenderer> m_BoxRenderer;
	std::shared_ptr<FrustumCuller> m_FrustumCuller;
	std::shared_ptr<Landscape> m_Landscape;

	std::vector<std::shared_ptr<GameObject>> m_GameObjects;

	std::chrono::steady_clock::time_point m_LastUpdate;
	double m_AppTime;
	double m_DeltaTime; // in seconds
	uint32_t m_FrameIndex;
	bool m_bShowCursor = false;
	bool m_bCursorToggleReleased = true;
	bool m_bShowBoundingBoxes = false;

	const char* m_QuadTexturePath = "Textures/image_gamma_linear.png";
	ID3D11ShaderResourceView* m_TextureResourceView;

	bool m_bRight = true;
};

#endif
