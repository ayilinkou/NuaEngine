#pragma once

#ifndef COMMON_H
#define COMMON_H

// if changing these, also update in Common.hlsl
#define MAX_POINT_LIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_PLANE_CHUNKS 1024
#define MAX_GRASS_PER_CHUNK 10000
#define MAX_INSTANCE_COUNT 1024
#define MAX_GRASS_COUNT (MAX_PLANE_CHUNKS * MAX_GRASS_PER_CHUNK)

#include <vector>
#include <utility>
#include <string>

#include "DirectXMath.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;
};

typedef unsigned long long UINT64;

struct RenderStats
{
	std::vector<std::pair<std::string, UINT64>> TrianglesRendered;
	std::vector<std::pair<std::string, UINT64>> InstancesRendered;
	UINT64 DrawCalls;
	UINT64 ComputeDispatches;
	double PostProcessPipelineTime;
	double FrameTime;
	double FPS;
};

inline struct ID3D11Buffer* NullBuffers[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
inline struct ID3D11RenderTargetView* NullRTVs[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
inline struct ID3D11ShaderResourceView* NullSRVs[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
inline struct ID3D11UnorderedAccessView* NullUAVs[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

#endif
