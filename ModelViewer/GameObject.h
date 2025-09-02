#pragma once

#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <unordered_map>

#include "Model.h"
#include "Component.h"

typedef unsigned int UINT;

class GameObject : public Component
{
public:
	GameObject();

	virtual void RenderControls() override;

	void SetName(const std::string& NewName);

	size_t GetUID() const { return m_UID; }
	const std::string& GetName() const { return m_Name; }

	static std::unordered_map<std::string, UINT>& GetNamesMap() { return ms_NamesMap; }

private:
	std::string m_Name;
	size_t m_UID;

	static size_t ms_UID;
	static std::unordered_map<std::string, UINT> ms_NamesMap;
};

#endif
