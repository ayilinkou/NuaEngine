#include "ImGui/imgui.h"

#include "Grass.h"
#include "ResourceManager.h"
#include "MyMacros.h"
#include "Graphics.h"
#include "Landscape.h"
#include "FrustumCuller.h"
#include "Camera.h"
#include "Common.h"
#include "Profiler.h"

typedef unsigned int UINT;

struct GrassVertex
{
	DirectX::XMFLOAT2 Position;
};

const GrassVertex GrassVertices[] =
{
	{{-0.028f, 0.0f}},
	{{ 0.028f, 0.0f}},
	{{-0.026f, 0.2f}},
	{{ 0.026f, 0.2f}},
	{{-0.024f, 0.4f}},
	{{ 0.024f, 0.4f}},
	{{-0.022f, 0.6f}},
	{{ 0.022f, 0.6f}},
	{{-0.020f, 0.8f}},
	{{ 0.020f, 0.8f}},
	{{ 0.000f, 1.0f}},
};

const GrassVertex GrassVerticesLOD[] =
{
	{{-0.028f, 0.0f}},
	{{ 0.028f, 0.0f}},
	{{ 0.000f, 1.0f}},
};

const UINT GrassIndices[] =
{
	0, 1, 2,
	3, 4, 5,
	6, 7, 8,
	9, 10
};

const UINT GrassIndicesLOD[] =
{
	0, 1, 2
};

Grass::Grass()
{
	SetName("Grass");
	SetWindDirection({ 1.f, 1.f });
	m_bShouldRender = true;
	m_LODDistanceThreshold = 80.f;
	m_Freq = 2.f;
	m_Amp = 1.5f;
	m_TimeScale = 4.f;
	m_FreqMultiplier = 1.4f;
	m_AmpMultiplier = 0.5f;
	m_WaveCount = 32u;
	m_WindStrength = 0.7f;
	m_SwayExponent = 1.5f;
	m_pLandscape = nullptr;
}

Grass::~Grass()
{
	Shutdown();
}

bool Grass::Init(Landscape* pLandscape, UINT GrassDimensionPerChunk, std::shared_ptr<FrustumCuller> pFrustumCuller,
	std::shared_ptr<Profiler> pProfiler)
{
	HRESULT hResult;
	bool Result;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;
	m_pLandscape = pLandscape;
	m_FrustumCuller = pFrustumCuller;
	m_Profiler = pProfiler;
	m_GrassPerChunk = GrassDimensionPerChunk * GrassDimensionPerChunk;
	assert(m_GrassPerChunk <= MAX_GRASS_PER_CHUNK);

	FALSE_IF_FAILED(CreateBuffers());

	m_vsFilepath = "Shaders/GrassVS.hlsl";
	m_psFilepath = "Shaders/GrassPS.hlsl";
	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilepath, "main", vsBuffer);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilepath);

	D3D11_INPUT_ELEMENT_DESC LayoutDesc[1] = {};
	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateInputLayout(LayoutDesc, 0u, vsBuffer->GetBufferPointer(),
		vsBuffer->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Grass input layout");

	GenerateAABB();

	return true;
}

void Grass::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilepath);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilepath);
}

void Grass::Render()
{
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	ID3D11DeviceContext* pContext = pGraphics->GetDeviceContext();
	m_FrustumCuller->CullGrass(
		m_GrassOffsetsSRV.Get(),
		m_BBox.Corners,
		m_GrassPerChunk,
		m_pLandscape->GetChunkInstanceCount(),
		m_pLandscape->GetChunkDimension(),
		m_pLandscape->GetHeightDisplacement(),
		m_LODDistanceThreshold,
		m_pLandscape->GetHeightmapSRV()
	);
	m_GrassInstanceCounts = m_FrustumCuller->GetInstanceCounts();
	m_FrustumCuller->SendInstanceCounts(m_ArgsBufferUAV, m_LODArgsBufferUAV);
	
	pGraphics->SetRasterStateBackFaceCull(false);

	UINT Strides[] = { 0u };
	UINT Offsets[] = { 0u };
	pContext->IASetInputLayout(m_InputLayout.Get());
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11Buffer* CBuffers[] = { m_pLandscape->m_LandscapeInfoCBuffer.Get(), m_GrassCBuffer.Get() };
	pContext->VSSetShader(m_VertexShader, nullptr, 0u);
	pContext->VSSetConstantBuffers(1u, 2u, CBuffers);
	pContext->VSSetShaderResources(0u, 1u, &m_pLandscape->m_HeightmapSRV);

	pContext->PSSetShader(m_PixelShader, nullptr, 0u);

	ID3D11Buffer* NullBuffers[] = { nullptr };
	pContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0u);
	pContext->IASetVertexBuffers(0u, 1u, NullBuffers, Strides, Offsets);

	// high LOD
	if (m_GrassInstanceCounts[0] > 0u)
	{
		pContext->VSSetShaderResources(1u, 1u, m_FrustumCuller->GetCulledGrassDataSRV().GetAddressOf());

		pContext->DrawInstancedIndirect(m_ArgsBuffer.Get(), 0u);
		m_Profiler->AddDrawCall();

		m_Profiler->AddTrianglesRendered("Grass", m_GrassInstanceCounts[0] * (_countof(GrassVertices) - 2));
		m_Profiler->AddInstancesRendered("Grass", m_GrassInstanceCounts[0]);
	}

	// low LOD
	if (m_GrassInstanceCounts[1] > 0u)
	{
		pContext->VSSetShaderResources(1u, 1u, m_FrustumCuller->GetCulledGrassLODDataSRV().GetAddressOf());

		pContext->DrawInstancedIndirect(m_LODArgsBuffer.Get(), 0u);
		m_Profiler->AddDrawCall();

		m_Profiler->AddTrianglesRendered("Grass LOD", m_GrassInstanceCounts[1] * (_countof(GrassVerticesLOD) - 2));
		m_Profiler->AddInstancesRendered("Grass LOD", m_GrassInstanceCounts[1]);
	}

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr };
	pContext->VSSetShaderResources(0u, 2u, NullSRVs);
}

void Grass::RenderControls()
{
	ImGui::Text(GetName().c_str());
	ImGui::Checkbox("Should Render Grass?", &m_bShouldRender);
	ImGui::SliderFloat("LOD Distance Threshold", &m_LODDistanceThreshold, 0.f, 1000.f);

	ImGui::Dummy(ImVec2(0.f, 10.f));

	ImGui::Text("Wind");
	
	bool bDirty = false;
	if (ImGui::SliderFloat("Frequency", &m_Freq, 0.f, 100.f))
	{
		bDirty = true;
	}
	if (ImGui::SliderFloat("Amplitude", &m_Amp, 0.f, 5.f))
	{
		bDirty = true;
	}

	DirectX::XMFLOAT2 WindDir = m_WindDir;
	if (ImGui::SliderFloat2("Wind Direction", reinterpret_cast<float*>(&WindDir), -1.f, 1.f))
	{
		SetWindDirection(WindDir);
		bDirty = true;
	}
	if (ImGui::SliderFloat("Time Scale", &m_TimeScale, 0.f, 10.f))
	{
		bDirty = true;
	}
	if (ImGui::SliderFloat("Frequency Multiplier", &m_FreqMultiplier, 1.f, 5.f))
	{
		bDirty = true;
	}
	if (ImGui::SliderFloat("Amplitude Multiplier", &m_AmpMultiplier, 0.f, 1.f))
	{
		bDirty = true;
	}

	UINT WaveCountMin = 0u;
	UINT WaveCountMax = 64u;
	if (ImGui::SliderScalar("Wave Count", ImGuiDataType_U32, &m_WaveCount, &WaveCountMin, &WaveCountMax))
	{
		bDirty = true;
	}
	if (ImGui::SliderFloat("Sway Height Exponent", &m_SwayExponent, 1.f, 10.f))
	{
		bDirty = true;
	}
	if (ImGui::SliderFloat("Wind Strength", &m_WindStrength, 0.f, 3.f))
	{
		bDirty = true;
	}

	if (bDirty)
	{
		UpdateBuffers();
	}
}

bool Grass::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = sizeof(WindCBuffer);
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	WindCBuffer WindData = {};
	WindData.Freq = m_Freq;
	WindData.Amp = m_Amp;
	WindData.Direction = m_WindDir;
	WindData.TimeScale = m_TimeScale;
	WindData.FreqMultiplier = m_FreqMultiplier;
	WindData.AmpMultiplier = m_AmpMultiplier;
	WindData.WaveCount = m_WaveCount;
	WindData.Strength = m_WindStrength;
	WindData.SwayExponent = m_SwayExponent;

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = &WindData;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_GrassCBuffer));
	NAME_D3D_RESOURCE(m_GrassCBuffer, "Grass constant buffer");

	Desc.CPUAccessFlags = 0u;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.ByteWidth = sizeof(DirectX::XMFLOAT2) * m_GrassPerChunk;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(DirectX::XMFLOAT2);

	Data.pSysMem = m_pLandscape->GetGrassOffsets().data();

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_GrassOffsetsBuffer));
	NAME_D3D_RESOURCE(m_GrassOffsetsBuffer, "Grass offsets buffer");

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.NumElements = m_GrassPerChunk;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(m_GrassOffsetsBuffer.Get(), &SRVDesc, &m_GrassOffsetsSRV));
	NAME_D3D_RESOURCE(m_GrassOffsetsSRV, "Grass offsets buffer SRV");

	Desc = {};
	Desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS ArgsData;
	ArgsData.IndexCountPerInstance = sizeof(GrassIndices) / sizeof(UINT);
	ArgsData.InstanceCount = 0u;
	ArgsData.StartIndexLocation = 0u;
	ArgsData.BaseVertexLocation = 0;
	ArgsData.StartInstanceLocation = 0u;

	Data = {};
	Data.pSysMem = &ArgsData;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_ArgsBuffer));
	NAME_D3D_RESOURCE(m_ArgsBuffer, "Grass args buffer");

	ArgsData.IndexCountPerInstance = sizeof(GrassIndicesLOD) / sizeof(UINT);
	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_LODArgsBuffer));
	NAME_D3D_RESOURCE(m_LODArgsBuffer, "Grass LOD args buffer");

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.NumElements = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) / 4;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateUnorderedAccessView(m_ArgsBuffer.Get(), &uavDesc, &m_ArgsBufferUAV));
	NAME_D3D_RESOURCE(m_ArgsBufferUAV, "Grass args buffer UAV");

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateUnorderedAccessView(m_LODArgsBuffer.Get(), &uavDesc, &m_LODArgsBufferUAV));
	NAME_D3D_RESOURCE(m_LODArgsBufferUAV, "Grass LOD args buffer UAV");
	
	return true;
}

void Grass::GenerateAABB()
{
	for (size_t i = 0; i < _countof(GrassVertices); i++)
	{
		m_BBox.Expand({ GrassVertices[i].Position.x * 20.f, GrassVertices[i].Position.y * 2.f, GrassVertices[i].Position.x * 20.f }); // could set this based off wind data, but hard coding for now
	}
	m_BBox.CalcCorners();
}

void Grass::UpdateBuffers()
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	WindCBuffer* CBufferPtr;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	ASSERT_NOT_FAILED(DeviceContext->Map(m_GrassCBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	CBufferPtr = (WindCBuffer*)MappedResource.pData;
	CBufferPtr->Freq = m_Freq;
	CBufferPtr->Amp = m_Amp;
	CBufferPtr->Direction = m_WindDir;
	CBufferPtr->TimeScale = m_TimeScale;
	CBufferPtr->FreqMultiplier = m_FreqMultiplier;
	CBufferPtr->AmpMultiplier = m_AmpMultiplier;
	CBufferPtr->WaveCount = m_WaveCount;
	CBufferPtr->Strength = m_WindStrength;
	CBufferPtr->SwayExponent = m_SwayExponent;
	DeviceContext->Unmap(m_GrassCBuffer.Get(), 0u);
}

void Grass::SetWindDirection(DirectX::XMFLOAT2 WindDir)
{
	DirectX::XMVECTOR v = DirectX::XMLoadFloat2(&WindDir);
	v = DirectX::XMVector2Normalize(v);
	DirectX::XMStoreFloat2(&m_WindDir, v);
}
