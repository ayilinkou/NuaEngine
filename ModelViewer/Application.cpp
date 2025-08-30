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
#include "DeferredShader.h"
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

	m_DeferredShader = std::make_unique<DeferredShader>();
	bResult = m_DeferredShader->Initialise(m_Graphics->GetDevice());
	assert(bResult);

	m_BoxRenderer = std::make_unique<BoxRenderer>();
	bResult = m_BoxRenderer->Init(m_CameraManager, m_Profiler);
	assert(bResult);

	m_Skybox = std::make_unique<Skybox>();
	bResult = m_Skybox->Init(m_CameraManager, m_Profiler);
	DirectX::XMFLOAT3 SkyColor = m_Skybox->GetAverageSkyColor();
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
	auto DirLight = std::make_shared<DirectionalLight>();
	DirLight->SetDiffuseColor(SkyColor.x, SkyColor.y, SkyColor.z);
	m_GameObjects.back()->AddComponent(DirLight);

	for (size_t i = 0; i < 1; i++)
	{
		m_GameObjects.emplace_back(std::make_shared<GameObject>());
		m_GameObjects.back()->SetName("Point Light_" + std::to_string(i));
		auto PLight = std::make_shared<PointLight>();
		PLight->SetIntensity(2.f);
		m_GameObjects.back()->AddComponent(PLight);
		m_GameObjects.back()->SetPosition(1.f, 3.f, 1.f);
		//m_GameObjects.back()->AddComponent(std::make_shared<Model>("Models/sphere.obj"));
		//auto& m = m_GameObjects.back()->GetModels()[0];
		//m->SetScale(0.1f); // TODO: still being set to 1.0 in ImGui for some reason, investigate more
	}

	m_GameObjects.emplace_back(std::make_shared<GameObject>());
	m_GameObjects.back()->SetPosition(0.f, 0.f, 0.f);
	m_GameObjects.back()->SetName("Car");
	m_GameObjects.back()->AddComponent(std::make_shared<Model>("Models/american_fullsize_73/scene.gltf", "Models/american_fullsize_73/"));

	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			m_GameObjects.emplace_back(std::make_shared<GameObject>());
			m_GameObjects.back()->SetPosition((float)i * 2.f, 15.f, (float)j * 2.f);
			m_GameObjects.back()->AddComponent(std::make_shared<Model>("Models/fantasy_sword_stylized/scene.gltf", "Models/fantasy_sword_stylized/"));
		}
	}


	return true;
}

void Application::Shutdown()
{		
	m_GameObjects.clear();

	m_Skybox.reset();
	m_InstancedShader.reset();
	m_DeferredShader.reset();
	m_Landscape.reset();
	m_FrustumCuller.reset();
	m_BoxRenderer.reset();
	m_CameraManager.reset();
	m_PostProcessManager.reset();

	ResourceManager::GetSingletonPtr()->Shutdown();

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
	// set first post process render target here
	bool DrawingForward = false; // true if drawing to RTV2 or from SRV1, false if drawing to RTV1 or from SRV2
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> CurrentRTV = m_Graphics->m_PostProcessRTVFirst;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SecondaryRTV = m_Graphics->m_PostProcessRTVSecond;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CurrentSRV = m_Graphics->m_PostProcessSRVFirst;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SecondarySRV = m_Graphics->m_PostProcessSRVSecond;

	m_Graphics->BeginScene(0.f, 0.f, 0.f, 1.f);
	m_Graphics->GetDeviceContext()->PSSetShaderResources(0u, 1u, NullSRVs);

	CullModels();

	RenderGeometryPass();
	RenderLightingPass(); // lighting pass needs to read from g buffer, so drawing direction will be flipped
	DrawingForward = !DrawingForward;

	RenderTransparencyPass();

	if (m_Skybox.get())
	{
		m_Skybox->Render(DrawingForward ? SecondaryRTV : CurrentRTV);
	}
	
	// apply post processes (if any) and keep track of which shader resource view is the latest
	DrawingForward = !DrawingForward;
	UINT Stride, Offset;
	Stride = IPostProcess::GetQuadVertexBufferStride();
	Offset = 0u;
	m_Graphics->GetDeviceContext()->IASetVertexBuffers(0u, 1u, IPostProcess::GetQuadVertexBuffer().GetAddressOf(), &Stride, &Offset);
	m_Graphics->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_Graphics->GetDeviceContext()->IASetInputLayout(IPostProcess::GetQuadInputLayout().Get());
	m_Graphics->GetDeviceContext()->IASetIndexBuffer(IPostProcess::GetQuadIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0u);
	m_Graphics->GetDeviceContext()->VSSetShader(IPostProcess::GetQuadVertexShader(), nullptr, 0u);
	m_Graphics->DisableDepthWriteAlwaysPass();
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
			m_BoxRenderer->LoadFrustumCorners(c);
	}

	if (m_bShowBoundingBoxes)
		RenderBoundingBoxes();

	m_BoxRenderer->Render();

	if (m_bShowCursor)
		RenderImGui();

	m_Graphics->EndScene();

	return true;
}

void Application::RenderGeometryPass()
{
	m_Graphics->EnableDepthWrite();
	ID3D11RenderTargetView* RTVs[3] = { m_Graphics->m_PostProcessRTVFirst.Get(), m_Graphics->GetNormalRTV().Get(), m_Graphics->GetVelocityRTV().Get() };
	m_Graphics->GetDeviceContext()->OMSetRenderTargets(3u, RTVs, m_Graphics->GetDepthStencilView());

	if (m_Landscape.get() && m_Landscape->ShouldRender())
	{
		m_Landscape->Render();
	}

	RenderOpaqueModels();
}

void Application::RenderLightingPass()
{
	ID3D11DeviceContext* pContext = m_Graphics->GetDeviceContext();

	// TODO: this is being copied
	UINT Stride, Offset;
	Stride = IPostProcess::GetQuadVertexBufferStride();
	Offset = 0u;
	pContext->IASetVertexBuffers(0u, 1u, IPostProcess::GetQuadVertexBuffer().GetAddressOf(), &Stride, &Offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetInputLayout(IPostProcess::GetQuadInputLayout().Get());
	pContext->IASetIndexBuffer(IPostProcess::GetQuadIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0u);
	pContext->VSSetShader(IPostProcess::GetQuadVertexShader(), nullptr, 0u);
	m_Graphics->DisableDepthWriteAlwaysPass();

	if (m_PostProcessManager->GetSSAO()->IsActive())
	{
		m_PostProcessManager->GetSSAO()->ApplyPostProcess(pContext, nullptr, nullptr);
	}
	
	pContext->OMSetRenderTargets(1u, m_Graphics->m_PostProcessRTVSecond.GetAddressOf(), nullptr);
	
	DeferredShader::ActivateLightingPassShader(m_Graphics->GetDeviceContext());
	ID3D11ShaderResourceView* SRVs[4] = { m_Graphics->m_PostProcessSRVFirst.Get(), m_Graphics->GetNormalSRV().Get(),
		m_PostProcessManager->GetSSAO()->GetVisibilitySRV(), m_Graphics->GetDepthStencilSRV().Get()};
	pContext->PSSetShaderResources(0u, 4u, SRVs);

	m_Graphics->GetDeviceContext()->DrawIndexed(6u, 0u, 0);
	m_Profiler->AddDrawCall();

	pContext->PSSetShaderResources(0u, 8u, NullSRVs);
}

void Application::RenderTransparencyPass()
{
	ID3D11RenderTargetView* RTVs[3] = { m_Graphics->m_PostProcessRTVSecond.Get(), m_Graphics->GetNormalRTV().Get(), m_Graphics->GetVelocityRTV().Get() };
	m_Graphics->GetDeviceContext()->OMSetRenderTargets(3u, RTVs, m_Graphics->GetDepthStencilView());
	RenderTransparentModels();
}

void Application::RenderOpaqueModels()
{	
	std::unordered_map<std::string, std::unique_ptr<Resource>>& Models = ResourceManager::GetSingletonPtr()->GetModelsMap();
	
	for (const auto& ModelPair : Models)
	{		
		ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
		if (!pModelData || pModelData->GetInstanceCount() == 0)
			continue;
		
		pModelData->RenderOpaque();
	}
}

void Application::RenderTransparentModels()
{
	std::unordered_map<std::string, std::unique_ptr<Resource>>& Models = ResourceManager::GetSingletonPtr()->GetModelsMap();

	for (const auto& ModelPair : Models)
	{
		ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
		if (!pModelData || pModelData->GetInstanceCount() == 0)
			continue;

		pModelData->RenderTransparent();
	}
}

void Application::RenderBoundingBoxes()
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
		float Scale = m_Landscape->GetScale().x;
		const AABB& BBox = m_Landscape->GetBoundingBox();
		/*for (const CullTransformData& Data : m_Landscape->GetChunkTransforms())
		{
			auto m = DirectX::XMMatrixTranspose(Data.CurrTransform); // back to column major
			m_BoxRenderer->LoadBoxCorners(BBox, m);

			const AABB& GrassBBox = m_Landscape->GetGrass()->GetBoundingBox();
			auto Origin = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f / Scale);
			DirectX::XMFLOAT3 ChunkOffset;
			DirectX::XMStoreFloat3(&ChunkOffset, DirectX::XMVector4Transform(Origin, m));
			for (const DirectX::XMFLOAT2& GrassData : m_Landscape->GetGrassOffsets())
			{
				auto WorldOffset = DirectX::XMMatrixTranslation(GrassData.x + ChunkOffset.x, 0.f, GrassData.y + ChunkOffset.z);
				auto InverseScale = DirectX::XMMatrixScaling(1.f / Scale, 1.f / Scale, 1.f / Scale);
				auto Final = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(WorldOffset, InverseScale), m); // doesn't account for height displacement

				m_BoxRenderer->LoadBoxCorners(GrassBBox, Final);
			}
		}*/

		for (int i = 0; i < m_Landscape->GetChunkOffsets().size(); ++i)
		{
			const DirectX::XMFLOAT2& Data = m_Landscape->GetChunkOffsets()[i];
			auto m = DirectX::XMMatrixMultiply(DirectX::XMMatrixScaling(Scale, 1.f, Scale), DirectX::XMMatrixTranslation(Data.x, 0.f, Data.y));
			m_BoxRenderer->LoadBoxCorners(BBox, m);

			const AABB& GrassBBox = m_Landscape->GetGrass()->GetBoundingBox();
			auto Origin = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f / Scale);
			DirectX::XMFLOAT3 ChunkOffset;
			DirectX::XMStoreFloat3(&ChunkOffset, DirectX::XMVector4Transform(Origin, m));
			for (const DirectX::XMFLOAT2& GrassData : m_Landscape->GetGrassOffsets())
			{
				auto WorldOffset = DirectX::XMMatrixTranslation(GrassData.x + ChunkOffset.x, 0.f, GrassData.y + ChunkOffset.z);
				auto InverseScale = DirectX::XMMatrixScaling(1.f / Scale, 1.f / Scale, 1.f / Scale);
				auto Final = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(WorldOffset, InverseScale), m); // works but everything is scaled, doesn't account for height displacement

				m_BoxRenderer->LoadBoxCorners(GrassBBox, Final);
			}
		}
	}
}

void Application::CullModels()
{
	std::unordered_map<std::string, std::unique_ptr<Resource>>& Models = ResourceManager::GetSingletonPtr()->GetModelsMap();

	for (const auto& ModelPair : Models)
	{
		ModelData* pModelData = static_cast<ModelData*>(ModelPair.second->GetDataPtr());
		if (!pModelData)
			continue;

		pModelData->ClearForCulling();
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

		pModelData->UpdateBuffers();

		// AABB frustum culling on transforms
		m_FrustumCuller->DispatchShaderNew(pModelData->GetCullData());
		m_Profiler->AddInstancesRendered(pModelData->GetModelPath(), pModelData->GetInstanceCount());
	}
}

void Application::RenderImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiManager::RenderPostProcessWindow(m_Profiler->GetRenderStats().PostProcessPipelineTime, m_PostProcessManager->GetPostProcesses(),
		m_PostProcessManager->GetSSAO());
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
	m_CameraManager->GetInverseViewMatrix(NewGlobalCBuffer.CameraData.InverseView);
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
