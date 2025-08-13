#include "Application.h"

#include <iostream>

#include "Windows.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#include "MyMacros.h"
#include "Common.h"
#include "Camera.h"
#include "InstancedShader.h"
#include "Model.h"
#include "Light.h"
#include "ResourceManager.h"
#include "SystemClass.h"
#include "InputClass.h"
#include "Skybox.h"
#include "Resource.h"
#include "ModelData.h"
#include "Landscape.h"
#include "BoxRenderer.h"
#include "FrustumCuller.h"
#include "TessellatedPlane.h"
#include "Grass.h"
#include "CameraManager.h"
#include "Profiler.h"
#include "PostProcess/PostProcess.h"
#include "PostProcess/PostProcessEmpty.h"
#include "PostProcess/PostProcessManager.h"
#include "GameObject.h"

Application* Application::m_Instance = nullptr;

Application::Application()
{
	m_LastUpdate = std::chrono::steady_clock::now();
	m_FrameIndex = 0u;
	m_AppTime = 0.0;
	m_hWnd = {};
	m_TextureResourceView = {};
}

bool Application::Initialise(int ScreenWidth, int ScreenHeight, HWND hWnd)
{
	m_hWnd = hWnd;
	bool bResult;

	m_Graphics = Graphics::GetSingletonPtr();
	bResult = m_Graphics->Initialise(ScreenWidth, ScreenHeight, VSYNC_ENABLED, hWnd, FULL_SCREEN, SCREEN_FAR, SCREEN_NEAR);
	assert(bResult);

	m_CameraManager = std::make_shared<CameraManager>();
	m_Profiler = std::make_shared<Profiler>(m_Graphics->GetDevice(), m_Graphics->GetDeviceContext());

	m_PostProcessManager = std::make_shared<PostProcessManager>();
	bResult = m_PostProcessManager->Init(m_Profiler, m_CameraManager);
	assert(bResult);
	m_CameraManager->SetPostProcessManager(m_PostProcessManager);
	
	m_FrustumCuller = std::make_shared<FrustumCuller>();
	bResult = m_FrustumCuller->Init(m_Profiler);
	assert(bResult);

	ResourceManager::GetSingletonPtr()->Init(hWnd, m_FrustumCuller.get(), m_Profiler);

	m_InstancedShader = std::make_unique<InstancedShader>();
	bResult = m_InstancedShader->Initialise(m_Graphics->GetDevice());
	assert(bResult);

	m_BoxRenderer = std::make_unique<BoxRenderer>();
	bResult = m_BoxRenderer->Init(m_CameraManager, m_Profiler);
	assert(bResult);

	m_Skybox = std::make_unique<Skybox>();
	bResult = m_Skybox->Init(m_CameraManager, m_Profiler);
	assert(bResult);

	UINT NumChunks = 16u;
	float ChunkSize = 25.f;
	float HeightDisplacement = 0.f;
	m_Landscape = std::make_shared<Landscape>(NumChunks, ChunkSize, HeightDisplacement, m_CameraManager);
	m_GameObjects.push_back(m_Landscape);

	float TessellationScale = 10.f;
	UINT GrassDimensionPerChunk = 32u;
	bResult = m_Landscape->Init("Textures/perlin_noise.png", TessellationScale, GrassDimensionPerChunk);
	assert(bResult);

	std::shared_ptr<Camera>& Camera = m_CameraManager->CreateCamera(m_Graphics->GetDefaultProjMatrix());
	m_CameraManager->SetMainCamera(Camera);
	Camera->SetPosition(0.f, 2.5f, -7.f);
	Camera->SetName("Main Camera");
	m_GameObjects.push_back(Camera);

	m_GameObjects.emplace_back(std::make_shared<GameObject>());
	m_GameObjects.back()->SetName("Directional Light");
	m_GameObjects.back()->AddComponent(std::make_shared<DirectionalLight>());

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			m_GameObjects.emplace_back(std::make_shared<GameObject>());
			m_GameObjects.back()->SetPosition((float)i * 2.f, 15.f, (float)j * 2.f);
			m_GameObjects.back()->AddComponent(std::make_shared<Model>("Models/fantasy_sword_stylized/scene.gltf", "Models/fantasy_sword_stylized/"));
		}
	}

	m_TextureResourceView = static_cast<ID3D11ShaderResourceView*>(ResourceManager::GetSingletonPtr()->LoadTexture(m_QuadTexturePath));

	return true;
}

void Application::Shutdown()
{		
	m_GameObjects.clear();

	m_Skybox.reset();
	m_InstancedShader.reset();
	m_Landscape.reset();
	m_FrustumCuller.reset();
	m_BoxRenderer.reset();
	m_CameraManager.reset();
	m_PostProcessManager.reset();

	ResourceManager::GetSingletonPtr()->UnloadTexture(m_QuadTexturePath);
	ResourceManager::GetSingletonPtr()->Shutdown();
	m_TextureResourceView = nullptr;

	if (m_Graphics)
	{
		m_Graphics->Shutdown();
		m_Graphics = nullptr;
	}
}

bool Application::Tick()
{	
	if (GetForegroundWindow() == m_hWnd)
	{
		ProcessInput();
	}

	auto Now = std::chrono::steady_clock::now();
	m_DeltaTime = std::chrono::duration_cast<std::chrono::microseconds>(Now - m_LastUpdate).count() / 1000000.0; // in seconds
	m_LastUpdate = Now;
	m_AppTime += m_DeltaTime;
	m_FrameIndex++;

	m_Profiler->Tick(m_DeltaTime);
	m_CameraManager->Tick(m_FrameIndex, m_Graphics->GetRenderTargetDimensions());
	m_BoxRenderer->ClearBoxes();

	for (const std::shared_ptr<GameObject>& GO : m_GameObjects)
	{
		GO->Tick(m_FrameIndex);
	}

	// frame time can differ significantly depending on if ImGui windows are shown
	std::cout << m_Profiler->GetRenderStats().FrameTime << std::endl;

	{
		// temp while testing how well motion vectors and color clamping are working for TAA
		const auto& Pos = m_GameObjects[3]->GetPosition();
		float BoundaryX = 5.f;

		if (Pos.x > BoundaryX)
		{
			m_GameObjects[3]->SetPosition(BoundaryX, Pos.y, Pos.z);
			m_bRight = false;
		}
		else if (Pos.x < -BoundaryX)
		{
			m_GameObjects[3]->SetPosition(-BoundaryX, Pos.y, Pos.z);
			m_bRight = true;
		}

		float Offset = m_bRight ? 1.f : -1.f;
		m_GameObjects[3]->SetPosition(Pos.x + Offset * (float)m_DeltaTime, Pos.y, Pos.z);
	}*/

	UpdateGlobalConstantBuffer();

	bool Result = Render();
	if (!Result)
	{
		return false;
	}
	
	return true;
}

bool Application::Render()
{			
	bool Result;
	
	// set first post process render target here
	bool DrawingForward = false; // true if drawing to RTV2 or from SRV1, false if drawing to RTV1 or from SRV2
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> CurrentRTV = m_Graphics->m_PostProcessRTVFirst;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SecondaryRTV = m_Graphics->m_PostProcessRTVSecond;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CurrentSRV = m_Graphics->m_PostProcessSRVFirst;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SecondarySRV = m_Graphics->m_PostProcessSRVSecond;

	m_Graphics->BeginScene(0.f, 0.f, 0.f, 1.f);
	m_Graphics->GetDeviceContext()->PSSetShaderResources(0u, 1u, NullSRVs);

	FALSE_IF_FAILED(RenderScene());
	//FALSE_IF_FAILED(RenderTexture(m_TextureResourceView));
	
	// apply post processes (if any) and keep track of which shader resource view is the latest
	DrawingForward = !DrawingForward;
	unsigned int Stride, Offset;
	Stride = sizeof(Vertex);
	Offset = 0u;
	m_Graphics->GetDeviceContext()->IASetVertexBuffers(0u, 1u, IPostProcess::GetQuadVertexBuffer().GetAddressOf(), &Stride, &Offset);
	m_Graphics->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Graphics->GetDeviceContext()->IASetInputLayout(IPostProcess::GetQuadInputLayout().Get());
	m_Graphics->GetDeviceContext()->IASetIndexBuffer(IPostProcess::GetQuadIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0u);
	m_Graphics->GetDeviceContext()->VSSetShader(IPostProcess::GetQuadVertexShader(), nullptr, 0u);
	m_Graphics->DisableDepthWriteAlwaysPass(); // simpler for now but might need to refactor when wanting to use depth data in post processes
	ApplyPostProcesses(CurrentRTV, SecondaryRTV, CurrentSRV, SecondarySRV, DrawingForward);

	// set back buffer as render target
	m_Graphics->SetBackBufferRenderTarget();

	// use last used post process texture to draw a full screen quad
	m_Graphics->SetRasterStateBackFaceCull(true);
	m_Graphics->GetDeviceContext()->PSSetShader(IPostProcess::GetEmptyPostProcess()->GetPixelShader().Get(), NULL, 0u);
	m_Graphics->GetDeviceContext()->PSSetShaderResources(0u, 1u, DrawingForward ? CurrentSRV.GetAddressOf() : SecondarySRV.GetAddressOf());
	m_Graphics->GetDeviceContext()->DrawIndexed(6u, 0u, 0);
	m_Profiler->AddDrawCall();

	for (const std::shared_ptr<Camera>& c : m_CameraManager->GetCameras())
	{
		if (c->ShouldVisualiseFrustum() && c.get() != m_CameraManager->GetActiveCamera().get())
		{
			m_BoxRenderer->LoadFrustumCorners(c);
		}
	}

	if (m_bShowBoundingBoxes)
	{
		// TODO: refactor this to also use the culled transforms
		std::unordered_map<std::string, std::unique_ptr<Resource>>& Models = ResourceManager::GetSingletonPtr()->GetModelsMap();
		for (const auto& ModelPair : Models)
		{
			ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
			if (!pModelData)
				continue;

			for (const auto& t : pModelData->GetTransforms())
			{
				m_BoxRenderer->LoadBoxCorners(pModelData->GetBoundingBox(), DirectX::XMMatrixTranspose(t.CurrTransform)); // back to column major
			}
		}

		if (m_Landscape->GetShouldRenderBBoxes())
		{
			const DirectX::XMMATRIX& Scale = m_Landscape->GetChunkScaleMatrix();
			const AABB& BBox = m_Landscape->GetBoundingBox();
			for (const DirectX::XMFLOAT2& o : m_Landscape->GetChunkOffsets())
			{
				DirectX::XMMATRIX m = DirectX::XMMatrixMultiply(Scale, DirectX::XMMatrixTranslation(o.x, 0.f, o.y));
				m_BoxRenderer->LoadBoxCorners(BBox, m);

				for (const DirectX::XMFLOAT2& GrassOffset : m_Landscape->GetGrassOffsets())
				{
					m_BoxRenderer->LoadBoxCorners(m_Landscape->GetGrass()->GetBoundingBox(),
						DirectX::XMMatrixMultiply(DirectX::XMMatrixTranslation(o.x, 0.f, o.y), DirectX::XMMatrixTranslation(GrassOffset.x, 0.f, GrassOffset.y))); // doesn't account for height displacement
				}
			}
		}
	}

	m_BoxRenderer->Render();

	if (m_bShowCursor)
		RenderImGui();

	m_Graphics->EndScene();

	return true;
}

bool Application::RenderScene()
{
	if (m_Skybox.get())
	{
		m_Skybox->Render(); // this should probably be rendered last to reduce overdraw
	}

	m_Graphics->EnableDepthWrite();
	ID3D11RenderTargetView* RTVs[3] = { m_Graphics->m_PostProcessRTVFirst.Get(), m_Graphics->GetNormalRTV().Get(), m_Graphics->GetVelocityRTV().Get() };
	m_Graphics->GetDeviceContext()->OMSetRenderTargets(3u, RTVs, m_Graphics->GetDepthStencilView());

	RenderModels();

	if (m_Landscape.get() && m_Landscape->ShouldRender())
	{
		m_Landscape->Render();
	}
	
	return true;
}

void Application::RenderModels()
{	
	DirectX::XMMATRIX View, Proj, ViewProj;
	const std::shared_ptr<Camera>& ActiveCamera = m_CameraManager->GetActiveCamera();
	ActiveCamera->GetViewMatrix(View);
	Proj = m_CameraManager->GetCurrJitteredProjMatrix();
	ViewProj = View * Proj;

	std::unordered_map<std::string, std::unique_ptr<Resource>>& Models = ResourceManager::GetSingletonPtr()->GetModelsMap();

	for (const auto& ModelPair : Models)
	{
		ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
		if (!pModelData)
			continue;

		pModelData->GetTransforms().clear();
	}

	for (auto& Object : m_GameObjects)
	{
		Object->SendTransformToModels();
	}
	
	for (const auto& ModelPair : Models)
	{		
		ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
		if (!pModelData || pModelData->GetTransforms().empty())
			continue;
		
		// AABB frustum culling on transforms
		
		UINT InstanceCount = m_FrustumCuller->GetInstanceCounts()[0];
		if (InstanceCount == 0)
			continue;
		
		pModelData->Render();
		m_Profiler->AddInstancesRendered(pModelData->GetModelPath(), InstanceCount);
	}
}

/*bool Application::RenderTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureView)
{
	unsigned int Stride, Offset;
	Stride = sizeof(Vertex);
	Offset = 0u;

	m_Graphics->GetDeviceContext()->IASetVertexBuffers(0u, 1u, IPostProcess::GetQuadVertexBuffer().GetAddressOf(), &Stride, &Offset);
	m_Graphics->GetDeviceContext()->IASetIndexBuffer(IPostProcess::GetQuadIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0u);
	m_Graphics->GetDeviceContext()->VSSetShader(IPostProcess::GetQuadVertexShader(), nullptr, 0u);
	
	m_Graphics->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Graphics->GetDeviceContext()->IASetInputLayout(m_Shader->GetInputLayout().Get());
	m_Graphics->GetDeviceContext()->PSSetShader(IPostProcess::GetEmptyPostProcess()->GetPixelShader().Get(), nullptr, 0u);
	m_Graphics->GetDeviceContext()->PSSetShaderResources(0, 1, TextureView.GetAddressOf());

	m_Graphics->GetDeviceContext()->DrawIndexed(6u, 0u, 0);
	m_Profiler->AddDrawCall();

	return true;
}*/

void Application::RenderImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiManager::RenderPostProcessWindow(m_Profiler->GetRenderStats().PostProcessPipelineTime, m_PostProcessManager->GetPostProcesses());
	ImGuiManager::RenderWorldHierarchyWindow();
	ImGuiManager::RenderCamerasWindow(m_CameraManager);
	ImGuiManager::RenderStatsWindow(m_Profiler->GetRenderStats());

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Application::ApplyPostProcesses(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> CurrentRTV, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SecondaryRTV,
										Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CurrentSRV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SecondarySRV, bool& DrawingForward)
{	
	ID3D11DeviceContext* pContext = m_Graphics->GetDeviceContext();
	m_Profiler->StartGPUTimer();
	
	auto& PostProcesses = m_PostProcessManager->GetPostProcesses();
	for (int i = 0; i < PostProcesses.size(); i++)
	{
		if (!PostProcesses[i]->GetIsActive())
		{
			continue;
		}

		PostProcesses[i]->ApplyPostProcess(pContext, DrawingForward ? SecondaryRTV : CurrentRTV,
												DrawingForward ? CurrentSRV : SecondarySRV);

		DrawingForward = !DrawingForward;
	}

	m_Profiler->EndGPUTimer();
	m_Profiler->SetPostProcessPipelineTime(m_Profiler->GetGPUTime());
}

void Application::UpdateGlobalConstantBuffer()
{
	const std::shared_ptr<Camera>& ActiveCamera = m_CameraManager->GetActiveCamera();
	GlobalCBuffer NewGlobalCBuffer = {};
	ActiveCamera->GetViewMatrix(NewGlobalCBuffer.CameraData.CurrView);
	ActiveCamera->GetProjMatrix(NewGlobalCBuffer.CameraData.CurrProj);
	ActiveCamera->GetViewProjMatrix(NewGlobalCBuffer.CameraData.CurrViewProj);
	m_CameraManager->GetCurrJitteredProjMatrix(NewGlobalCBuffer.CameraData.CurrProjJittered);
	m_CameraManager->GetCurrJitteredViewProjMatrix(NewGlobalCBuffer.CameraData.CurrViewProjJittered);
	m_CameraManager->GetInverseProjMatrix(NewGlobalCBuffer.CameraData.InverseProj);
	m_CameraManager->ExtractFrustumPlanes(NewGlobalCBuffer.CameraData.FrustumPlanes);
	NewGlobalCBuffer.CameraData.ActiveCameraPos = ActiveCamera->GetPosition();
	NewGlobalCBuffer.CameraData.MainCameraPos = m_CameraManager->GetMainCamera()->GetPosition();
	NewGlobalCBuffer.CurrTime = (float)m_AppTime;
	NewGlobalCBuffer.NearZ = SCREEN_NEAR;
	NewGlobalCBuffer.FarZ = SCREEN_FAR;
	NewGlobalCBuffer.LightData.SkylightColor = m_Skybox->GetAverageSkyColor();
	m_Graphics->UpdateGlobalConstantBuffer(NewGlobalCBuffer);
}

void Application::ProcessInput()
{
	if (InputClass::GetSingletonPtr()->IsKeyDown('M'))
	{
		if (m_bCursorToggleReleased)
		{
			m_bCursorToggleReleased = false;
			ToggleShowCursor();
		}
	}
	else
	{
		m_bCursorToggleReleased = true;
	}

	if (m_bShowCursor)
	{
		return;
	}
	
	const CameraSettings& CameraSettings = m_CameraManager->GetCameraSettings();
	const std::shared_ptr<Camera>& ActiveCamera = m_CameraManager->GetActiveCamera();
	DirectX::XMFLOAT3 LookDir = ActiveCamera->GetRotatedLookDir();
	DirectX::XMFLOAT3 LookRight = ActiveCamera->GetRotatedLookRight();
	float EffectiveCameraSpeed = CameraSettings.CameraSpeed * (float)m_DeltaTime;
	
	if (GetAsyncKeyState('W') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x + LookDir.x * EffectiveCameraSpeed, ActiveCamera->GetPosition().y + LookDir.y * EffectiveCameraSpeed, ActiveCamera->GetPosition().z + LookDir.z * EffectiveCameraSpeed);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x - LookDir.x * EffectiveCameraSpeed, ActiveCamera->GetPosition().y - LookDir.y * EffectiveCameraSpeed, ActiveCamera->GetPosition().z - LookDir.z * EffectiveCameraSpeed);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x + LookRight.x * EffectiveCameraSpeed, ActiveCamera->GetPosition().y, ActiveCamera->GetPosition().z + LookRight.z * EffectiveCameraSpeed);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x - LookRight.x * EffectiveCameraSpeed, ActiveCamera->GetPosition().y, ActiveCamera->GetPosition().z - LookRight.z * EffectiveCameraSpeed);
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x, ActiveCamera->GetPosition().y - 1.f * EffectiveCameraSpeed, ActiveCamera->GetPosition().z);
	}
	if (GetAsyncKeyState('E') & 0x8000)
	{
		ActiveCamera->SetPosition(ActiveCamera->GetPosition().x, ActiveCamera->GetPosition().y + 1.f * EffectiveCameraSpeed, ActiveCamera->GetPosition().z);
	}

	ActiveCamera->SetRotation(ActiveCamera->GetRotation().x + SystemClass::m_MouseDelta.y * 0.1f, ActiveCamera->GetRotation().y + SystemClass::m_MouseDelta.x * 0.1f, 0.f);

	short Delta = InputClass::GetSingletonPtr()->GetMouseWheelDelta();
	if (Delta > 0)
	{
		m_CameraManager->SetCameraSpeed(std::fmin(CameraSettings.CameraSpeed * (float)(Delta / 60), CameraSettings.CameraSpeedMax));
	}
	else if (Delta < 0)
	{
		m_CameraManager->SetCameraSpeed(std::fmax(CameraSettings.CameraSpeed * (1.f / (float)(Delta / 60)), CameraSettings.CameraSpeedMin));
	}
}

void Application::ToggleShowCursor()
{	
	if (m_bShowCursor)
	{
		int Count;
		do {
			Count = ShowCursor(FALSE);
		} while (Count >= 0);
		SystemClass::ms_bShouldProcessMouse = true;
		SetCursorPos(SystemClass::ms_Center.x, SystemClass::ms_Center.y);
		SystemClass::m_MouseDelta = { 0.f, 0.f };
	}
	else
	{
		int Count;
		do {
			Count = ShowCursor(TRUE);
		} while (Count < 0);
		SystemClass::ms_bShouldProcessMouse = false;
		SystemClass::m_MouseDelta = { 0.f, 0.f };
	}
	m_bShowCursor = !m_bShowCursor;

	std::cout << (m_bShowCursor ? "ImGui opened!" : "ImGui closed!") << std::endl;
}
