#include "Model.h"

#include "ResourceManager.h"
#include "ModelData.h"
#include "ImGui/imgui.h"

Model::Model(const std::string& ModelPath, const std::string& TexturesPath)
{
	m_pModelData = ResourceManager::GetSingletonPtr()->LoadModel(ModelPath, TexturesPath);
	assert(m_pModelData);

	m_bShouldRender = true;
	m_ComponentName = "Model";
}

Model::~Model()
{
	Shutdown();
}

void Model::Shutdown()
{
	if (!m_pModelData)
	{
		return;
	}

	ResourceManager::GetSingletonPtr()->UnloadModel(m_pModelData->GetModelPath());
	m_pModelData = nullptr;
}

void Model::RenderControls()
{
	ImGui::Text(m_ComponentName.c_str());
	ImGui::Checkbox("Should Render", &m_bShouldRender);
}

void Model::SendTransformToModel()
{
	DirectX::XMMATRIX Transform = DirectX::XMMatrixTranspose(GetAccumulatedWorldMatrix());
	DirectX::XMMATRIX PrevTransform = DirectX::XMMatrixTranspose(GetPrevAccumulatedWorldMatrix());
	m_pModelData->GetTransforms().push_back({ Transform, PrevTransform });
}
