#include "ResourceManager.h"

#include <cassert>
#include <algorithm>

#pragma warning(push)
#pragma warning(disable : 26819)
#pragma warning(disable : 6262)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma warning(pop)

#include "Graphics.h"
#include "ModelData.h"
#include "MyMacros.h"

ResourceManager* ResourceManager::ms_Instance = nullptr;

ResourceManager* ResourceManager::GetSingletonPtr()
{
	if (!ResourceManager::ms_Instance)
	{
		ResourceManager::ms_Instance = new ResourceManager();
	}
	return ResourceManager::ms_Instance;
}

bool ResourceManager::Init(HWND hWnd)
{
	m_hWnd = hWnd;
	return true;
}

void ResourceManager::Shutdown()
{
	if (!m_TexturesMap.empty() || !m_ModelsMap.empty() || !m_ShadersMap.empty())
	{
		__debugbreak(); // Attempting to shutdown when resources are still loaded!
	}

	m_TexturesMap.clear();
	m_ModelsMap.clear();
	m_ShadersMap.clear();
}

ID3D11ShaderResourceView* ResourceManager::LoadTexture(const std::string& Filepath)
{
	auto it = m_TexturesMap.find(Filepath);
	if (it != m_TexturesMap.end() && it->second.get())
	{
		it->second->AddRef();
		return static_cast<ID3D11ShaderResourceView*>(it->second->m_pData);
	}

	ID3D11ShaderResourceView* pData = Internal_LoadTexture(Filepath.c_str());
	if (!pData)
	{
		return nullptr;
	}

	m_TexturesMap[Filepath] = std::make_unique<Resource>(pData);
	return pData;
}

ModelData* ResourceManager::LoadModel(const std::string& ModelPath, const std::string& TexturesPath)
{
	auto it = m_ModelsMap.find(ModelPath);
	if (it != m_ModelsMap.end() && it->second.get())
	{
		it->second->AddRef();
		return static_cast<ModelData*>(it->second->m_pData);
	}

	ModelData* pData = Internal_LoadModel(ModelPath.c_str(), TexturesPath.c_str());
	if (!pData)
	{
		return nullptr;
	}

	m_ModelsMap[ModelPath] = std::make_unique<Resource>(pData);
	return pData;
}

UINT ResourceManager::UnloadTexture(const std::string& ModelPath)
{
	Resource* ResourceToUnload = m_TexturesMap[ModelPath].get();
	if (!ResourceToUnload)
	{
		m_TexturesMap.erase(ModelPath);
		return 0;
	}

	ResourceToUnload->RemoveRef();
	if (ResourceToUnload->m_RefCount > 0)
	{
		return ResourceToUnload->m_RefCount;
	}

	Internal_UnloadTexture(ModelPath);
	return 0;
}

UINT ResourceManager::UnloadModel(const std::string& Filepath)
{
	Resource* ResourceToUnload = m_ModelsMap[Filepath].get();
	if (!ResourceToUnload)
	{
		m_ModelsMap.erase(Filepath);
		return 0;
	}

	ResourceToUnload->RemoveRef();
	if (ResourceToUnload->m_RefCount > 0)
	{
		return ResourceToUnload->m_RefCount;
	}

	Internal_UnloadModel(Filepath);
	return 0;
}

ID3D11ShaderResourceView* ResourceManager::Internal_LoadTexture(const char* Filepath)
{
	int Width, Height, Channels;
	std::string FileString(Filepath);
	unsigned char* ImageData = stbi_load(Filepath, &Width, &Height, &Channels, 0);
	assert(ImageData);

	unsigned char* ImageDataRgba = ImageData;
	bool NeedsAlpha = Channels == 3;

	ID3D11Texture2D* Texture;
	ID3D11ShaderResourceView* TextureView = nullptr;

	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (Channels == 1)
	{
		TexDesc.Format = DXGI_FORMAT_R8_UNORM;
	}
	else if (Channels == 2)
	{
		TexDesc.Format = DXGI_FORMAT_R8G8_UNORM;
	}
	else if (Channels == 3)
	{
		ImageDataRgba = new unsigned char[Width * Height * 4];

		for (int i = 0; i < Width * Height; i++)
		{
			ImageDataRgba[i * 4 + 0] = ImageData[i * 3 + 0];
			ImageDataRgba[i * 4 + 1] = ImageData[i * 3 + 1];
			ImageDataRgba[i * 4 + 2] = ImageData[i * 3 + 2];
			ImageDataRgba[i * 4 + 3] = 255;
		}

		Channels = 4;
		TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = ImageDataRgba;
	InitData.SysMemPitch = Width * Channels;

	assert(SUCCEEDED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TexDesc, &InitData, &Texture)));
	assert(SUCCEEDED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(Texture, NULL, &TextureView)));

	if (NeedsAlpha)
	{
		delete[] ImageDataRgba;
	}
	stbi_image_free(ImageData);

	NAME_D3D_RESOURCE(Texture, (FileString + " texture").c_str());
	NAME_D3D_RESOURCE(TextureView, (FileString + " texture SRV").c_str());

	Texture->Release();

	return TextureView;
}

ModelData* ResourceManager::Internal_LoadModel(const char* ModelPath, const char* TexturesPath)
{
	ModelData* pData = new ModelData(ModelPath, TexturesPath);
	
	return pData;
}

void ResourceManager::Internal_UnloadTexture(const std::string& Filepath)
{
	ID3D11ShaderResourceView* SRV = static_cast<ID3D11ShaderResourceView*>(m_TexturesMap[Filepath]->m_pData);
	SRV->Release();
	m_TexturesMap.erase(Filepath);
}

void ResourceManager::Internal_UnloadModel(const std::string& Filepath)
{
	ModelData* pModelData = static_cast<ModelData*>(m_ModelsMap[Filepath]->m_pData);
	pModelData->Shutdown();
	m_ModelsMap.erase(Filepath);
}
