#pragma once

#ifndef PROFILER_H
#define PROFILER_H

#include <vector>
#include <string>
#include <utility>

#include "wrl.h"

#include "d3d11.h"

typedef unsigned long long UINT64;

struct RenderStats
{
	std::vector<std::pair<std::string, UINT64>> TrianglesRendered;
	std::vector<std::pair<std::string, UINT64>> InstancesRendered;
	UINT64 DrawCalls = 0u;
	UINT64 ComputeDispatches = 0u;
	double PostProcessPipelineTime = 0.0;
	double FrameTime = 0.0;
	double FPS = 0.0;
};

class Profiler
{
public:
	Profiler(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void ClearRenderStats();
	void SetFPS(double FPS) { m_RenderStats.FPS = FPS; }
	void SetFrameTime(double FrameTime) { m_RenderStats.FrameTime = FrameTime; }
	void SetPostProcessPipelineTime(double PostProcessPipelineTime) { m_RenderStats.PostProcessPipelineTime = PostProcessPipelineTime; }
	void AddTrianglesRendered(const std::string& Name, UINT64 TriangleCount) { m_RenderStats.TrianglesRendered.push_back({ Name, TriangleCount }); }
	void AddInstancesRendered(const std::string& Name, UINT64 InstanceCount) { m_RenderStats.InstancesRendered.push_back({ Name, InstanceCount }); }
	void AddDrawCall() { m_RenderStats.DrawCalls++; }
	void AddComputeDispatch() { m_RenderStats.ComputeDispatches++; }

	void StartGPUTimer();
	void EndGPUTimer();
	double GetGPUTime();

	const RenderStats& GetRenderStats() { return m_RenderStats; }

private:
	bool SetupQueries();

private:
	RenderStats m_RenderStats;
	Microsoft::WRL::ComPtr<ID3D11Query> m_DisjointQuery;
	Microsoft::WRL::ComPtr<ID3D11Query> m_TimestampStart;
	Microsoft::WRL::ComPtr<ID3D11Query> m_TimestampEnd;

	ID3D11Device* m_Device;
	ID3D11DeviceContext* m_Context;

};

#endif
