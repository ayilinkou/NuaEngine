#include "d3dcompiler.h"

#include "ImGui/imgui.h"

#include "TessellatedPlane.h"
#include "MyMacros.h"
#include "Graphics.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Common.h"
#include "Landscape.h"
#include "FrustumCuller.h"
#include "BoxRenderer.h"
#include "CameraManager.h"
#include "Profiler.h"

struct PlaneVertex
{
	DirectX::XMFLOAT3 Position;
};

const PlaneVertex ChunkVertices[] =
{
	{{-0.5f, 0.f, -0.5f}},
	{{-0.5f, 0.f,  0.5f}},
	{{ 0.5f, 0.f, -0.5f}},
	{{ 0.5f, 0.f,  0.5f}},
};

const UINT ChunkIndices[] =
{
	0, 1, 2, 3
};

TessellatedPlane::TessellatedPlane()
{
	SetName("Tessellated Plane");
	m_bShouldRender = true;
	m_TessellationScale = 1.f;
}

TessellatedPlane::~TessellatedPlane()
{
	Shutdown();
}

bool TessellatedPlane::Init(float TessellationScale, Landscape* pLandscape, std::shared_ptr<FrustumCuller> pFrustumCuller,
	std::shared_ptr<Profiler> pProfiler)
{
	bool Result;
	FALSE_IF_FAILED(CreateShaders());
	FALSE_IF_FAILED(CreateBuffers());
	
	m_TessellationScale = TessellationScale;
	m_pLandscape = pLandscape;
	m_FrustumCuller = pFrustumCuller;
	m_Profiler = pProfiler;
	
	return true;
}

void TessellatedPlane::Render(const std::shared_ptr<CameraManager>& CamManager)
{
	m_FrustumCuller->SendInstanceCounts(m_ArgsBufferUAV);

	Graphics::GetSingletonPtr()->EnableDepthWrite();
	Graphics::GetSingletonPtr()->DisableBlending();
	UpdateBuffers(CamManager);

	Graphics* pGraphics = Graphics::GetSingletonPtr();
	ID3D11DeviceContext* DeviceContext = pGraphics->GetDeviceContext();
	UINT Strides[] = { sizeof(PlaneVertex) };
	UINT Offsets[] = { 0u };

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	DeviceContext->IASetInputLayout(m_InputLayout.Get());
	DeviceContext->IASetVertexBuffers(0u, 1u, m_VertexBuffer.GetAddressOf(), Strides, Offsets);
	DeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

	ID3D11ShaderResourceView* vsSRVs[] = { m_pLandscape->m_HeightmapSRV, m_FrustumCuller->GetCulledOffsetsSRV().Get() };
	DeviceContext->VSSetShader(m_VertexShader, nullptr, 0u);
	DeviceContext->VSSetConstantBuffers(1u, 1u, m_pLandscape->m_LandscapeInfoCBuffer.GetAddressOf());
	DeviceContext->VSSetShaderResources(0u, 2u, vsSRVs);

	DeviceContext->HSSetShader(m_HullShader, nullptr, 0u);
	DeviceContext->HSSetConstantBuffers(1u, 1u, m_HullCBuffer.GetAddressOf());

	DeviceContext->DSSetShader(m_DomainShader, nullptr, 0u);
	DeviceContext->DSSetConstantBuffers(1u, 1u, m_pLandscape->m_LandscapeInfoCBuffer.GetAddressOf());
	DeviceContext->DSSetShaderResources(0u, 1u, &m_pLandscape->m_HeightmapSRV);

	// this doesn't actually do that much now since chunk culling is so effective
	//DeviceContext->GSSetShader(m_GeometryShader, nullptr, 0u);
	//DeviceContext->GSSetConstantBuffers(1u, 1u, m_pLandscape->m_CullingCBuffer.GetAddressOf());

	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_pLandscape->m_LandscapeInfoCBuffer.GetAddressOf());
	DeviceContext->PSSetShaderResources(0u, 1u, &m_pLandscape->m_HeightmapSRV);

	DeviceContext->Begin(pGraphics->GetPipelineStatsQuery().Get());
	DeviceContext->DrawIndexedInstancedIndirect(m_ArgsBuffer.Get(), 0u);
	DeviceContext->End(pGraphics->GetPipelineStatsQuery().Get());
	m_Profiler->AddDrawCall();

	D3D11_QUERY_DATA_PIPELINE_STATISTICS Stats = {};
	while (DeviceContext->GetData(pGraphics->GetPipelineStatsQuery().Get(), &Stats, sizeof(Stats), 0) != S_OK)
	{
		// sleep or maybe do it on next frame
	}

	m_Profiler->AddTrianglesRendered("Tessellated Plane", Stats.GSPrimitives);
	m_Profiler->AddInstancesRendered("Tessellated Plane chunks", m_pLandscape->m_ChunkInstanceCount);

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr };
	DeviceContext->VSSetShaderResources(0u, 2u, NullSRVs);

	DeviceContext->VSSetShader(nullptr, nullptr, 0u);
	DeviceContext->HSSetShader(nullptr, nullptr, 0u);
	DeviceContext->DSSetShader(nullptr, nullptr, 0u);
	DeviceContext->GSSetShader(nullptr, nullptr, 0u);
	DeviceContext->PSSetShader(nullptr, nullptr, 0u);
}

void TessellatedPlane::Shutdown()
{
	m_InputLayout.Reset();
	m_IndexBuffer.Reset();
	m_VertexBuffer.Reset();
	m_HullCBuffer.Reset();

	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11HullShader>(m_hsFilename);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11DomainShader>(m_dsFilename);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11GeometryShader>(m_gsFilename);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void TessellatedPlane::RenderControls()
{
	ImGui::Text(GetName().c_str());

	ImGui::DragFloat("Tessellation Scale", &m_TessellationScale, 0.1f, 1.f, 256.f, "%.f", ImGuiSliderFlags_AlwaysClamp);

	ImGui::Checkbox("Should Render Plane?", &m_bShouldRender);
}

bool TessellatedPlane::CreateShaders()
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;

	m_vsFilename = "Shaders/TessellatedPlaneVS.hlsl";
	m_hsFilename = "Shaders/TessellatedPlaneHS.hlsl";
	m_dsFilename = "Shaders/TessellatedPlaneDS.hlsl";
	m_gsFilename = "Shaders/TessellatedPlaneGS.hlsl";
	m_psFilename = "Shaders/TessellatedPlanePS.hlsl";

	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilename, "main", vsBuffer);
	m_HullShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11HullShader>(m_hsFilename);
	m_DomainShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11DomainShader>(m_dsFilename);
	m_GeometryShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11GeometryShader>(m_gsFilename);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilename);
	
	D3D11_INPUT_ELEMENT_DESC LayoutDesc[1] = {};
	LayoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	LayoutDesc[0].SemanticName = "POSITION";
	LayoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateInputLayout(LayoutDesc, _countof(LayoutDesc), vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Tessellated plane input layout");

	return true;
}

bool TessellatedPlane::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	Desc.ByteWidth = sizeof(ChunkIndices);

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = ChunkIndices;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_IndexBuffer));
	NAME_D3D_RESOURCE(m_IndexBuffer, "Tessellated plane index buffer");

	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.ByteWidth = sizeof(ChunkVertices);

	Data.pSysMem = ChunkVertices;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_VertexBuffer));
	NAME_D3D_RESOURCE(m_VertexBuffer, "Tessellated plane vertex buffer");

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.ByteWidth = sizeof(HullCBuffer);

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &m_HullCBuffer));
	NAME_D3D_RESOURCE(m_HullCBuffer, "Tessellated plane hull constant buffer");

	Desc = {};
	Desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS ArgsData;
	ArgsData.IndexCountPerInstance = sizeof(ChunkIndices) / sizeof(UINT);
	ArgsData.InstanceCount = 0u;
	ArgsData.StartIndexLocation = 0u;
	ArgsData.BaseVertexLocation = 0;
	ArgsData.StartInstanceLocation = 0u;

	Data = {};
	Data.pSysMem = &ArgsData;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_ArgsBuffer));
	NAME_D3D_RESOURCE(m_ArgsBuffer, "Tessellated plane args buffer");

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.NumElements = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) / 4;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateUnorderedAccessView(m_ArgsBuffer.Get(), &uavDesc, &m_ArgsBufferUAV));
	NAME_D3D_RESOURCE(m_ArgsBufferUAV, "Tessellated plane args buffer UAV");
	
	return true;
}

void TessellatedPlane::UpdateBuffers(const std::shared_ptr<CameraManager>& CamManager)
{	
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HullCBuffer* HullCBufferPtr;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
		
	ASSERT_NOT_FAILED(DeviceContext->Map(m_HullCBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	HullCBufferPtr = (HullCBuffer*)MappedResource.pData;
	HullCBufferPtr->CameraPos = CamManager->GetMainCamera()->GetPosition();
	HullCBufferPtr->TessellationScale = m_TessellationScale;
	DeviceContext->Unmap(m_HullCBuffer.Get(), 0u);
}
