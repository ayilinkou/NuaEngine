#include <random>

#include "ImGui/imgui.h"

#include "Landscape.h"
#include "TessellatedPlane.h"
#include "MyMacros.h"
#include "Graphics.h"
#include "Application.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Common.h"
#include "Grass.h"
#include "FrustumCuller.h"
#include "CameraManager.h"

Landscape::Landscape(UINT ChunkDimension, float ChunkSize, float HeightDisplacement, std::shared_ptr<CameraManager> CamManager)
	: m_ChunkDimension(ChunkDimension), m_ChunkSize(ChunkSize), m_NumChunks(ChunkDimension * ChunkDimension),
	m_HeightDisplacement(HeightDisplacement), m_CameraManager(CamManager)
{
	SetName("Landscape");
	SetScale(ChunkSize);
	m_bShouldRender = true;
	m_bShouldRenderBBoxes = true;
	m_bVisualiseChunks = false;
	m_HeightmapSRV = nullptr;
	m_ChunkInstanceCount = 0u;
	assert(m_NumChunks >= 0 && m_NumChunks <= MAX_PLANE_CHUNKS);
}

Landscape::~Landscape()
{
	Shutdown();
}

bool Landscape::Init(const std::string& HeightMapFilepath, float TessellationScale, UINT GrassDimensionPerChunk)
{
	bool Result;
	Application* pApp = Application::GetSingletonPtr();

	SetupAABB();
	GenerateChunkOffsets();
	GenerateGrassOffsets(GrassDimensionPerChunk);

	FALSE_IF_FAILED(CreateBuffers());

	m_Plane = std::make_shared<TessellatedPlane>();
	FALSE_IF_FAILED(m_Plane->Init(TessellationScale, this, pApp->GetFrustumCuller(), pApp->GetProfiler()));

	m_Grass = std::make_shared<Grass>();
	FALSE_IF_FAILED(m_Grass->Init(this, GrassDimensionPerChunk, pApp->GetFrustumCuller(), pApp->GetProfiler()));

	m_HeightmapSRV = ResourceManager::GetSingletonPtr()->LoadTexture(HeightMapFilepath);
	assert(m_HeightmapSRV);

	m_HeightMapFilepath = HeightMapFilepath;

	return true;
}

void Landscape::SetupAABB()
{
	m_BoundingBox.Min = { -0.5f * m_Transform.Scale.x, 0.f, -0.5f * m_Transform.Scale.z };
	m_BoundingBox.Max = { 0.5f * m_Transform.Scale.x, m_HeightDisplacement, 0.5f * m_Transform.Scale.z };
	m_BoundingBox.CalcCorners();
}

void Landscape::Render()
{	
	Application* pApp = Application::GetSingletonPtr();
	pApp->GetFrustumCuller()->DispatchShader(m_ChunkOffsets, m_BoundingBox);
	m_ChunkInstanceCount = pApp->GetFrustumCuller()->GetInstanceCounts()[0];
	
	if (m_ChunkInstanceCount == 0u)
		return;

	UpdateBuffers();

	if (m_Plane->ShouldRender())
	{
		m_Plane->Render(m_CameraManager);
	}

	if (m_Grass->ShouldRender())
	{
		m_Grass->Render();
	}
}

void Landscape::Shutdown()
{
	m_LandscapeInfoCBuffer.Reset();
	m_Plane.reset();
	m_Grass.reset();

	ResourceManager::GetSingletonPtr()->UnloadTexture(m_HeightMapFilepath);
	m_HeightmapSRV = nullptr;
}

void Landscape::RenderControls()
{
	ImGui::Text("Landscape");
	
	float HeightDisplacement = m_HeightDisplacement;
	if (ImGui::DragFloat("Height Displacement", &HeightDisplacement, 0.1f, 0.f, 256.f, "%.f", ImGuiSliderFlags_AlwaysClamp))
	{
		SetHeightDisplacement(HeightDisplacement);
	}

	ImGui::Checkbox("Should Render?", &m_bShouldRender);
	ImGui::Checkbox("Should Render Bounding Boxes?", &m_bShouldRenderBBoxes);
	ImGui::Checkbox("Visualise Chunks?", &m_bVisualiseChunks);

	ImGui::Dummy(ImVec2(0.f, 10.f));

	m_Plane->RenderControls();

	ImGui::Dummy(ImVec2(0.f, 10.f));

	m_Grass->RenderControls();
}

void Landscape::SetHeightDisplacement(float NewHeight)
{
	m_HeightDisplacement = NewHeight;
	SetupAABB();
}

bool Landscape::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};

	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.ByteWidth = sizeof(DirectX::XMFLOAT2) * m_NumChunks;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(DirectX::XMFLOAT2);

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = m_ChunkOffsets.data();

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_ChunkOffsetsBuffer));
	NAME_D3D_RESOURCE(m_ChunkOffsetsBuffer, "Landscape chunk offsets buffer");

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.NumElements = m_NumChunks;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(m_ChunkOffsetsBuffer.Get(), &SRVDesc, &m_ChunkOffsetsSRV));
	NAME_D3D_RESOURCE(m_ChunkOffsetsSRV, "Chunk offsets buffer SRV");

	Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.ByteWidth = sizeof(LandscapeInfoCBuffer);

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &m_LandscapeInfoCBuffer));
	NAME_D3D_RESOURCE(m_LandscapeInfoCBuffer, "Landscape info constant buffer");

	return true;
}

void Landscape::UpdateBuffers()
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	LandscapeInfoCBuffer* LandscapeInfoCBufferPtr;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	ASSERT_NOT_FAILED(DeviceContext->Map(m_LandscapeInfoCBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	LandscapeInfoCBufferPtr = (LandscapeInfoCBuffer*)MappedResource.pData;
	LandscapeInfoCBufferPtr->PlaneDimension = (float)m_ChunkDimension * m_ChunkSize;
	LandscapeInfoCBufferPtr->HeightDisplacement = m_HeightDisplacement;
	LandscapeInfoCBufferPtr->bVisualiseChunks = m_bVisualiseChunks;
	LandscapeInfoCBufferPtr->ChunkInstanceCount = m_ChunkInstanceCount;
	LandscapeInfoCBufferPtr->GrassPerChunk = m_Grass->GetGrassPerChunk();
	LandscapeInfoCBufferPtr->Padding = {};
	LandscapeInfoCBufferPtr->ChunkScaleMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(m_ChunkSize, m_ChunkSize, m_ChunkSize));

	DeviceContext->Unmap(m_LandscapeInfoCBuffer.Get(), 0u);
}

void Landscape::GenerateChunkOffsets()
{
	float ChunkHalf = m_ChunkSize / 2.f;
	float HalfCount = (float)m_ChunkDimension / 2.f;
	int chunkID = 0;

	m_ChunkOffsets.resize(m_NumChunks, { 0.f, 0.f });
	for (UINT z = 0; z < m_ChunkDimension; z++)
	{
		float WorldZ = ((int)z - HalfCount) * m_ChunkSize + m_ChunkSize * 0.5f;
		for (UINT x = 0; x < m_ChunkDimension; x++)
		{
			float WorldX = ((int)x - HalfCount) * m_ChunkSize + m_ChunkSize * 0.5f;

			m_ChunkOffsets[chunkID++] = { WorldX, WorldZ };
		}
	}
}

void Landscape::GenerateGrassOffsets(UINT GrassCount)
{
	if (GrassCount <= 1)
	{
		m_GrassOffsets.push_back({ 0.f, 0.f });
		return;
	}

	float SpacingX = m_ChunkSize / (GrassCount);
	float SpacingZ = m_ChunkSize / (GrassCount);

	float HalfWidth = m_ChunkSize * 0.5f;
	float HalfDepth = m_ChunkSize * 0.5f;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> TranslationDist(0.f, SpacingX);
	std::uniform_real_distribution<float> RotationDist(0.f, 360.f);

	for (UINT x = 0; x < GrassCount; ++x)
	{
		for (UINT z = 0; z < GrassCount; ++z)
		{
			float RandOffsetX = TranslationDist(gen);
			float RandOffsetZ = TranslationDist(gen);
			
			float WorldX = -HalfWidth + x * SpacingX + RandOffsetX;
			float WorldZ = -HalfDepth + z * SpacingZ + RandOffsetZ;

			assert(m_GrassOffsets.size() < MAX_GRASS_PER_CHUNK);

			m_GrassOffsets.push_back({ WorldX, WorldZ });
		}
	}
}
