#include <fstream>

#include "PostProcessManager.h"
#include "PostProcess.h"
#include "JsonParser.h"

PostProcessManager::~PostProcessManager()
{
	Shutdown();
}

void PostProcessManager::Shutdown()
{
	IPostProcess::ShutdownStatics();
	m_PostProcesses.clear();
	m_pTAA = nullptr;
}

bool PostProcessManager::Init(std::shared_ptr<Profiler> pProfiler, std::shared_ptr<CameraManager> pCameraManager)
{
	IPostProcess::InitStatics(pProfiler);

	std::ifstream File("config.json");
	if (!File)
	{
		throw std::runtime_error("config.json not found!");
	}
	nlohmann::json Config;
	File >> Config;

	const std::pair<int, int>& Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
	DirectX::XMFLOAT2 PixelSize = DirectX::XMFLOAT2(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second);

	PostProcessData::BloomData BloomData = {};
	PostProcessData::GaussianBlurData BloomGaussianData = {};
	PostProcessData::BoxBlurData BoxBlurData = {};
	PostProcessData::ChromaticAberrationData ChromaticData = {};
	PostProcessData::ColorCorrectionData ColorData = {};
	PostProcessData::FogData FogData = {};
	PostProcessData::GammaCorrectionData GammaData = {};
	PostProcessData::GaussianBlurData GaussianBlurData = {};
	PostProcessData::PixelationData PixelationData = {};
	PostProcessData::TemporalAAData TAAData = {};
	PostProcessData::ToneMapperData ToneMapperData = {};
	bool bBloomActive;
	bool bBoxBlurActive;
	bool bChromaticActive;
	bool bColorActive;
	bool bFogActive;
	bool bGammaActive;
	bool bGaussianBlurActive;
	bool bPixelationActive;
	bool bTAAActive;
	bool bToneMapperActive;

	for (const auto& PPConfig : Config["postProcesses"])
	{
		const std::string Type = PPConfig["type"];
		if (Type == "Bloom")
		{
			BloomData.LuminanceThreshold = PPConfig.value("luminanceThreshold", 0.9f);
			BloomGaussianData.TexelSize = PixelSize;
			BloomGaussianData.BlurStrength = PPConfig.value("blurStrength", 16);
			BloomGaussianData.Sigma = PPConfig.value("sigma", 8.f);
			bBloomActive = PPConfig.value("active", false);
		}
		else if (Type == "BoxBlur")
		{
			BoxBlurData.BlurStrength = PPConfig.value("blurStrength", 30);
			BoxBlurData.TexelSize = PixelSize;
			bBoxBlurActive = PPConfig.value("active", false);
		}
		else if (Type == "ChromaticAberration")
		{
			ChromaticData.Scale = PPConfig.value("scale", 1.f);
			ChromaticData.PixelSize = PixelSize;
			bChromaticActive = PPConfig.value("active", false);
		}
		else if (Type == "ColorCorrection")
		{
			ColorData.Contrast = PPConfig.value("contrast", 1.f);
			ColorData.Brightness = PPConfig.value("brightness", 0.f);
			ColorData.Saturation = PPConfig.value("saturation", 1.f);
			bColorActive = PPConfig.value("active", true);
		}
		else if (Type == "Fog")
		{
			JsonParser::ParseFloat3(PPConfig.value("color", std::vector<float>{0.8f, 0.8f, 0.8f}), FogData.FogColor);
			FogData.Formula = JsonParser::StringToFogFormula(PPConfig.value("formula", "ExponentialSquared"));
			FogData.Density = PPConfig.value("density", 0.005f);
			FogData.NearPlane = Graphics::GetSingletonPtr()->GetNearPlane();
			FogData.FarPlane = Graphics::GetSingletonPtr()->GetFarPlane();
			bFogActive = PPConfig.value("active", true);
		}
		else if (Type == "GammaCorrection")
		{
			GammaData.Gamma = PPConfig.value("gamma", 2.2f);
			bGammaActive = PPConfig.value("active", true);
		}
		else if (Type == "GaussianBlur")
		{
			GaussianBlurData.BlurStrength = PPConfig.value("blurStrength", 30);
			GaussianBlurData.Sigma = PPConfig.value("sigma", 4.f);
			GaussianBlurData.TexelSize = PixelSize;
			bGaussianBlurActive = PPConfig.value("active", false);
		}
		else if (Type == "Pixelation")
		{
			PixelationData.PixelSize = PPConfig.value("pixelSize", 8.f);
			PixelationData.TruePixelSize = PixelSize;
			bPixelationActive = PPConfig.value("active", false);
		}
		else if (Type == "TAA")
		{
			TAAData.Alpha = PPConfig.value("alpha", 0.5f);
			bTAAActive = PPConfig.value("active", true);
		}
		else if (Type == "ToneMapper")
		{
			ToneMapperData.WhiteLevel = PPConfig.value("whiteLevel", 1.5f);
			ToneMapperData.Exposure = PPConfig.value("exposure", 1.f);
			ToneMapperData.Bias = PPConfig.value("bias", 1.f);
			ToneMapperData.Formula = JsonParser::StringToToneMapperFormula(PPConfig.value("formula", "HillACES"));
			bToneMapperActive = PPConfig.value("active", true);
		}
		else
		{
			throw std::runtime_error("Post process type not found in config.json!");
		}
	}

	m_PostProcesses.emplace_back(std::make_unique<PostProcessFog>(bFogActive, FogData));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessTemporalAA>(bTAAActive, TAAData, pCameraManager));
	m_pTAA = m_PostProcesses.back().get();
	m_PostProcesses.emplace_back(std::make_unique<PostProcessPixelation>(bPixelationActive, PixelationData));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessBoxBlur>(bBoxBlurActive, BoxBlurData, Dimensions));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessGaussianBlur>(bGaussianBlurActive, GaussianBlurData, Dimensions));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessBloom>(bBloomActive, BloomData, BloomGaussianData, Dimensions));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessChromaticAberration>(bChromaticActive, ChromaticData));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessToneMapper>(bToneMapperActive, ToneMapperData));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessColorCorrection>(bColorActive, ColorData));
	m_PostProcesses.emplace_back(std::make_unique<PostProcessGammaCorrection>(bGammaActive, GammaData));

	return true;
}

bool PostProcessManager::IsUsingTAA() const
{
	return m_pTAA && m_pTAA->IsActive();
}
