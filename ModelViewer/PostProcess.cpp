#include "PostProcess.h"

ID3D11VertexShader* PostProcess::ms_QuadVertexShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11InputLayout> PostProcess::ms_QuadInputLayout;
Microsoft::WRL::ComPtr<ID3D11Buffer> PostProcess::ms_QuadVertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> PostProcess::ms_QuadIndexBuffer;
std::shared_ptr<PostProcessEmpty> PostProcess::ms_EmptyPostProcess;
std::shared_ptr<Profiler> PostProcess::ms_Profiler;
const char* PostProcess::ms_vsFilename = "Shaders/QuadVS.hlsl";
bool PostProcess::ms_bInitialised = false;