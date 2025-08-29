#pragma once

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>

#include "d3d11.h"
#include "d3dcompiler.h"

#include "Resource.h"
#include "ShaderResource.h"
#include "ShaderCreateInfo.h"
#include "MyMacros.h"
#include "Logger.h"
#include "Graphics.h"

class ModelData;
class FrustumCuller;
class Profiler;

class ResourceManager
{
private:
	ResourceManager() : m_hWnd(0) {}

	static ResourceManager* ms_Instance;

public:
	static ResourceManager* GetSingletonPtr();

	bool Init(HWND hWnd, FrustumCuller* pFrustumCuller, std::shared_ptr<Profiler> pProfiler);

	void Shutdown();

	// these must NOT be stored with a ComPtr and should be unloaded using UnloadTexture when no longer needed
	ID3D11ShaderResourceView* LoadTexture(const std::string& Filepath);
	ModelData* LoadModel(const std::string& ModelPath, const std::string& TexturesPath);
	template <typename T>
	T* LoadShader(const std::string& Filepath, const std::string& Entry = "main");
	template <typename T>
	T* LoadShader(const std::string& Filepath, const std::string& Entry, Microsoft::WRL::ComPtr<ID3D10Blob>& Bytecode);

	UINT UnloadTexture(const std::string& Filepath);
	UINT UnloadModel(const std::string& Filepath);
	template <typename T>
	UINT UnloadShader(const std::string& Filepath, const std::string& Entry = "main");

	std::unordered_map<std::string, std::unique_ptr<Resource>>& GetTexturesMap() { return m_TexturesMap; }
	std::unordered_map<std::string, std::unique_ptr<Resource>>& GetModelsMap() { return m_ModelsMap; }
	std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ShaderResource>>>& GetShadersMap() { return m_ShadersMap; }

private:
	ID3D11ShaderResourceView* Internal_LoadTexture(const char* Filepath);
	ModelData* Internal_LoadModel(const char* ModelPath, const char* TexturesPath);
	template <typename T>
	T* Internal_LoadShader(const char* Filepath, const char* Entry, Microsoft::WRL::ComPtr<ID3D10Blob>& Bytecode);

	void Internal_UnloadTexture(const std::string& Filepath);
	void Internal_UnloadModel(const std::string& Filepath);
	template <typename T>
	void Internal_UnloadShader(const std::string& Filepath, const std::string& Entry);

private:
	std::unordered_map<std::string, std::unique_ptr<Resource>> m_TexturesMap;
	std::unordered_map<std::string, std::unique_ptr<Resource>> m_ModelsMap;
	std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ShaderResource>>> m_ShadersMap;

	HWND m_hWnd;
	FrustumCuller* m_FrustumCuller = nullptr;
	std::shared_ptr<Profiler> m_Profiler;

};

template<typename T>
inline T* ResourceManager::LoadShader(const std::string& Filepath, const std::string& Entry)
{
	//assert(Type >= 0 && Type < ShaderType::None);
	
	auto it = m_ShadersMap.find(Filepath);
	if (it != m_ShadersMap.end())
	{
		auto iter = it->second.find(Entry);
		if (iter != it->second.end())
		{
			iter->second->ShaderRes->AddRef();
			return static_cast<T*>(iter->second->ShaderRes->m_pData);
		}
	}

	m_ShadersMap[Filepath][Entry] = std::make_unique<ShaderResource>();
	T* pData = Internal_LoadShader<T>(Filepath.c_str(), Entry.c_str(), m_ShadersMap[Filepath][Entry]->Bytecode);
	if (!pData)
	{
		return nullptr;
	}

	m_ShadersMap[Filepath][Entry]->ShaderRes = std::make_unique<Resource>(pData);

	return pData;
}

template<typename T>
inline T* ResourceManager::LoadShader(const std::string& Filepath, const std::string& Entry, Microsoft::WRL::ComPtr<ID3D10Blob>& Bytecode)
{
	T* Ptr = LoadShader<T>(Filepath, Entry);
	assert(Ptr);
	Bytecode = m_ShadersMap[Filepath][Entry]->Bytecode;
	return Ptr;
}

template<typename T>
inline UINT ResourceManager::UnloadShader(const std::string& Filepath, const std::string& Entry)
{
	auto& pShader = m_ShadersMap[Filepath][Entry];
	if (!pShader.get() || !pShader->ShaderRes.get())
	{
		__debugbreak(); // attempting to unload a shader which isn't loaded
	}

	Resource* ResourceToUnload = pShader->ShaderRes.get();
	if (!ResourceToUnload)
	{
		__debugbreak(); // attempting to unload a shader which isn't loaded
		m_ShadersMap[Filepath].erase(Entry);
		if (m_ShadersMap[Filepath].empty())
		{
			m_ShadersMap.erase(Filepath);
		}
		return 0u;
	}

	ResourceToUnload->RemoveRef();
	if (ResourceToUnload->m_RefCount > 0)
	{
		return ResourceToUnload->m_RefCount;
	}

	Internal_UnloadShader<T>(Filepath, Entry);
	return 0u;
}

template<typename T>
inline T* ResourceManager::Internal_LoadShader(const char* Filepath, const char* Entry, Microsoft::WRL::ComPtr<ID3D10Blob>& Bytecode)
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> ErrorMessage;

	std::wstring WideString;
	int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, Filepath, -1, nullptr, 0);
	WideString.resize(SizeNeeded - 1);
	MultiByteToWideChar(CP_UTF8, 0, Filepath, -1, &WideString[0], SizeNeeded);

	const WCHAR* WideFilepath = WideString.c_str();

	UINT CompileFlags = D3D10_SHADER_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	hResult = D3DCompileFromFile(WideFilepath, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, Entry, ShaderCreateInfo<T>::Target, CompileFlags, 0, &Bytecode, &ErrorMessage);
	if (FAILED(hResult))
	{
		if (ErrorMessage.Get())
		{
			Logger::OutputShaderErrorMessage(ErrorMessage.Get(), m_hWnd, WideFilepath);
		}
		else
		{
			MessageBox(m_hWnd, WideFilepath, L"Missing shader file!", MB_OK);
		}

		return nullptr;
	}

	T* ShaderPtr;
	std::string Path(Filepath);
	ASSERT_NOT_FAILED(ShaderCreateInfo<T>::Create(Graphics::GetSingletonPtr()->GetDevice(), Bytecode.Get(), &ShaderPtr));
	NAME_D3D_RESOURCE(ShaderPtr, (Path + " " + Entry + ShaderCreateInfo<T>::Suffix).c_str());
	
	return ShaderPtr;
}

template<typename ShaderType>
inline void ResourceManager::Internal_UnloadShader(const std::string& Filepath, const std::string& Entry)
{
	ShaderType* Shader = static_cast<ShaderType*>(m_ShadersMap[Filepath][Entry]->ShaderRes->m_pData);
	Shader->Release();
	m_ShadersMap[Filepath].erase(Entry);

	if (m_ShadersMap[Filepath].empty())
	{
		m_ShadersMap.erase(Filepath);
	}
}

#endif
