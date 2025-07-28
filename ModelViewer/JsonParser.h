#pragma once

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "DirectXMath.h"

#include "nlohmann/json.hpp"

// TODO: doing these forward declares forces me to include post process header in cpp, is there a better way?
namespace PostProcessData
{
	enum class FogFormula;
	enum class ToneMapperFormula;
};

class JsonParser
{
public:
	static void ParseFloat2(const nlohmann::json& j, DirectX::XMFLOAT2& v);
	static void ParseFloat3(const nlohmann::json& j, DirectX::XMFLOAT3& v);
	static PostProcessData::FogFormula StringToFogFormula(const std::string& str);
	static PostProcessData::ToneMapperFormula StringToToneMapperFormula(const std::string& str);
};

#endif
