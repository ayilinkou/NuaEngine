#include "d3dcompiler.h"

#include "Skybox.h"
#include "Graphics.h"
#include "MyMacros.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "CameraManager.h"
#include "Profiler.h"

struct CubeVertex
{
	DirectX::XMFLOAT3 Position;
};

const CubeVertex CubeVertices[] =
{
	{{-0.5f,  0.5f, -0.5f}},
	{{ 0.5f,  0.5f, -0.5f}},
	{{ 0.5f, -0.5f, -0.5f}},
	{{-0.5f, -0.5f, -0.5f}},
	{{-0.5f,  0.5f,  0.5f}},
	{{ 0.5f,  0.5f,  0.5f}},
	{{ 0.5f, -0.5f,  0.5f}},
	{{-0.5f, -0.5f,  0.5f}},
};

const UINT CubeIndices[] =
{
	2, 1, 0, 0, 3, 2, // -Z
	1, 5, 4, 4, 0, 1, // +Y
	5, 6, 7, 7, 4, 5, // +Z
	6, 2, 3, 3, 7, 6, // -Y
	6, 5, 1, 1, 2, 6, // +X
	3, 0, 4, 4, 7, 3  // -X
};

Skybox::~Skybox()
{
	Shutdown();
}

bool Skybox::Init(std::shared_ptr<CameraManager> CamManager, std::shared_ptr<Profiler> pProfiler)
{
	HRESULT hResult;
	D3D11_TEXTURE2D_DESC CubeDesc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	m_CameraManager = CamManager;
	m_Profiler = pProfiler;
	m_vsFilename = "Shaders/SkyboxVS.hlsl";
	m_psFilename = "Shaders/SkyboxPS.hlsl";

	LoadTextures();
	m_Textures[0]->GetDesc(&CubeDesc);
	CubeDesc.ArraySize = 6u;
	CubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	std::vector<std::vector<BYTE>> TextureData(6);
	D3D11_SUBRESOURCE_DATA Data[6] = {};
	for (int i = 0; i < 6; i++)
	{
		ID3D11Texture2D* OriginalTex = m_Textures[i];
		D3D11_TEXTURE2D_DESC Desc;
		OriginalTex->GetDesc(&Desc);

		Desc.Usage = D3D11_USAGE_STAGING;
		Desc.BindFlags = 0;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		Desc.MiscFlags = 0;

		ID3D11Texture2D* StagingTex = nullptr;
		HFALSE_IF_FAILED(Device->CreateTexture2D(&Desc, nullptr, &StagingTex));
		StagingTex->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Skybox staging texture"), "Skybox staging texture");
		DeviceContext->CopyResource(StagingTex, OriginalTex);

		D3D11_MAPPED_SUBRESOURCE Mapped;
		DeviceContext->Map(StagingTex, 0, D3D11_MAP_READ, 0, &Mapped);

		size_t DataSize = Mapped.RowPitch * Desc.Height;
		TextureData[i].resize(DataSize);
		memcpy(TextureData[i].data(), Mapped.pData, DataSize);

		DeviceContext->Unmap(StagingTex, 0);
		StagingTex->Release();

		Data[i].pSysMem = TextureData[i].data();
		Data[i].SysMemPitch = Mapped.RowPitch;
	}

	for (auto* Tex : m_Textures)
	{
		Tex->Release();
	}
	m_Textures.clear();

	for (const std::string& Filename : m_FileNames)
	{
		ResourceManager::GetSingletonPtr()->UnloadTexture(m_TexturesDir + Filename);
	}

	HFALSE_IF_FAILED(Device->CreateTexture2D(&CubeDesc, Data, &m_CubeTexture));
	NAME_D3D_RESOURCE(m_CubeTexture, "Skybox cube texture");

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = CubeDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SRVDesc.Texture2D.MostDetailedMip = 0u;
	SRVDesc.Texture2D.MipLevels = 1u;
	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CubeTexture.Get(), &SRVDesc, &m_SRV));
	NAME_D3D_RESOURCE(m_SRV, "Skybox SRV");

	bool Result;
	FALSE_IF_FAILED(CreateBuffers());

	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;

	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilename, "main", vsBuffer);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilename);

	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HFALSE_IF_FAILED(Device->CreateInputLayout(Layout, 1u, vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Skybox input layout");

	CalculateAverageSkyColor(TextureData);
	
	return true;
}

void Skybox::Render(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& RTV)
{
	UINT Strides[] = { sizeof(CubeVertex) };
	UINT Offsets[] = { 0u };

	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), pGraphics->GetDepthStencilView());
	DeviceContext->IASetInputLayout(m_InputLayout.Get());
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->IASetVertexBuffers(0u, 1u, m_VertexBuffer.GetAddressOf(), Strides, Offsets);
	DeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);
	DeviceContext->VSSetShader(m_VertexShader, nullptr, 0u);
	DeviceContext->VSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, m_SRV.GetAddressOf());

	Graphics::GetSingletonPtr()->DisableDepthWritePassLessEqual();
	Graphics::GetSingletonPtr()->SetRasterStateBackFaceCull(true);

	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;

	struct BufferData
	{
		DirectX::XMMATRIX Matrix;
	};
	BufferData* DataPtr;

	DirectX::XMMATRIX View, Proj, ViewProj;
	m_CameraManager->GetActiveCamera()->GetViewMatrix(View);
	Proj = m_CameraManager->GetCurrJitteredProjMatrix();

	View.r[3] = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f); // removes translation from the view matrix
	ViewProj = View * Proj;
	
	// remember to transpose from row major before sending to shaders
	ViewProj = DirectX::XMMatrixTranspose(ViewProj);

	ASSERT_NOT_FAILED(DeviceContext->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	DataPtr = (BufferData*)MappedResource.pData;
	DataPtr->Matrix = ViewProj;
	DeviceContext->Unmap(m_ConstantBuffer.Get(), 0u);

	DeviceContext->DrawIndexed(sizeof(CubeIndices) / sizeof(UINT), 0u, 0);
	m_Profiler->AddDrawCall();

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr };
	DeviceContext->PSSetShaderResources(0u, 1u, NullSRVs);
}

void Skybox::Shutdown()
{
	m_SRV.Reset();
	m_CubeTexture.Reset();
	m_InputLayout.Reset();
	m_VertexBuffer.Reset();
	m_IndexBuffer.Reset();
	m_ConstantBuffer.Reset();

	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_psFilename);
}

bool Skybox::LoadTextures()
{
	for (const std::string& Filename : m_FileNames)
	{
		ID3D11ShaderResourceView* SRV = ResourceManager::GetSingletonPtr()->LoadTexture(m_TexturesDir + Filename);
		if (!SRV)
		{
			return false;
		}
		
		ID3D11Resource* Resource = nullptr;
		SRV->GetResource(&Resource);

		ID3D11Texture2D* Texture = nullptr;
		HRESULT hResult;
		
		HFALSE_IF_FAILED(Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&Texture));
		//Texture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)(m_TexturesDir + Filename).length(), (m_TexturesDir + Filename).c_str());
		m_Textures.push_back(Texture);

		Resource->Release();
	}
	return true;
}

bool Skybox::CreateBuffers()
{
	HRESULT hResult;

	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.ByteWidth = sizeof(CubeVertices);
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = CubeVertices;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_VertexBuffer));
	NAME_D3D_RESOURCE(m_VertexBuffer, "Skybox vertex buffer");

	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.ByteWidth = sizeof(CubeIndices);
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	Data.pSysMem = CubeIndices;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_IndexBuffer));
	NAME_D3D_RESOURCE(m_IndexBuffer, "Skybox index buffer");

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = sizeof(DirectX::XMMATRIX);
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &m_ConstantBuffer));
	NAME_D3D_RESOURCE(m_ConstantBuffer, "Skybox constant buffer");
	
	return true;
}

void Skybox::CalculateAverageSkyColor(const std::vector<std::vector<BYTE>>& TextureData)
{
	DirectX::XMFLOAT3 TotalColor = { 0.f, 0.f, 0.f };
	size_t TotalPixels = 0;

	// TODO: only sample upper hemisphere
	for (const auto& Face : TextureData)
	{
		for (size_t i = 0; i < Face.size(); i += 4)
		{
			float r = Face[i + 0] / 255.f;
			float g = Face[i + 1] / 255.f;
			float b = Face[i + 2] / 255.f;

			TotalColor.x += r;
			TotalColor.y += g;
			TotalColor.z += b;
			TotalPixels++;
		}
	}

	assert(TotalPixels > 0);
	m_AverageSkyColor = { TotalColor.x / (float)TotalPixels, TotalColor.y / (float)TotalPixels, TotalColor.z / (float)TotalPixels };
}
