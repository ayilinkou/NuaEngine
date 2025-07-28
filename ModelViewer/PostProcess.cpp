#include "PostProcess.h"

ID3D11VertexShader* IPostProcess::ms_QuadVertexShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11InputLayout> IPostProcess::ms_QuadInputLayout;
Microsoft::WRL::ComPtr<ID3D11Buffer> IPostProcess::ms_QuadVertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> IPostProcess::ms_QuadIndexBuffer;
std::shared_ptr<PostProcessEmpty> IPostProcess::ms_EmptyPostProcess;
std::shared_ptr<Profiler> IPostProcess::ms_Profiler;
const char* IPostProcess::ms_vsFilename = "Shaders/QuadVS.hlsl";
bool IPostProcess::ms_bInitialised = false;