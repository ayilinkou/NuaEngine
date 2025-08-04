#include "JsonParser.h"
#include "PostProcess/PostProcess.h"

void JsonParser::ParseFloat2(const nlohmann::json& j, DirectX::XMFLOAT2& v)
{
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
}

void JsonParser::ParseFloat3(const nlohmann::json& j, DirectX::XMFLOAT3& v)
{
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
    j.at(2).get_to(v.z);
}

PostProcessData::FogFormula JsonParser::StringToFogFormula(const std::string& str)
{
    if (str == "Linear") return PostProcessData::FogFormula::Linear;
	if (str == "Exponential") return PostProcessData::FogFormula::Exponential;
	if (str == "ExponentialSquared") return PostProcessData::FogFormula::ExponentialSquared;
	throw std::invalid_argument("Invalid FogFormula string");
}

PostProcessData::ToneMapperFormula JsonParser::StringToToneMapperFormula(const std::string& str)
{
    if (str == "ReinhardBasic") return PostProcessData::ToneMapperFormula::ReinhardBasic;
    if (str == "ReinhardExtended") return PostProcessData::ToneMapperFormula::ReinhardExtended;
    if (str == "ReinhardExtendedBias") return PostProcessData::ToneMapperFormula::ReinhardExtendedBias;
    if (str == "NarkowiczACES") return PostProcessData::ToneMapperFormula::NarkowiczACES;
    if (str == "HillACES") return PostProcessData::ToneMapperFormula::HillACES;
    throw std::invalid_argument("Invalid ToneMapperFormula string");
}
