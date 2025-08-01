#include "Profiler.h"
#include "MyMacros.h"

Profiler::Profiler(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) : m_Device(pDevice), m_Context(pContext)
{
	bool Result = SetupQueries();
	assert(Result);
}

void Profiler::Tick(double DeltaTime)
{
	ClearRenderStats();
	m_RenderStats.FrameTime = (DeltaTime * 1000.0);
	m_RenderStats.FPS = (1.0 / DeltaTime);
}

void Profiler::ClearRenderStats()
{
	m_RenderStats.TrianglesRendered.clear();
	m_RenderStats.InstancesRendered.clear();
	m_RenderStats = {};
}

void Profiler::StartGPUTimer()
{
	m_Context->Begin(m_DisjointQuery.Get());
	m_Context->End(m_TimestampStart.Get());
}

void Profiler::EndGPUTimer()
{
	m_Context->End(m_TimestampEnd.Get());
	m_Context->End(m_DisjointQuery.Get());
}

double Profiler::GetGPUTime()
{
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData;
	while (m_Context->GetData(m_DisjointQuery.Get(), &DisjointData, sizeof(DisjointData), 0) != S_OK);

	// only calculate if the GPU clock didn't change (eg. due to GPU frequency scaling event)
	if (!DisjointData.Disjoint)
	{
		UINT64 StartTime = 0, EndTime = 0;
		while (m_Context->GetData(m_TimestampStart.Get(), &StartTime, sizeof(StartTime), 0) != S_OK);
		while (m_Context->GetData(m_TimestampEnd.Get(), &EndTime, sizeof(EndTime), 0) != S_OK);

		return (EndTime - StartTime) / double(DisjointData.Frequency) * 1000.0;
	}
	return 0.0;
}

bool Profiler::SetupQueries()
{
	HRESULT hResult;
	D3D11_QUERY_DESC Desc = {};
	Desc.Query = D3D11_QUERY_TIMESTAMP;
	HFALSE_IF_FAILED(m_Device->CreateQuery(&Desc, &m_TimestampStart));
	HFALSE_IF_FAILED(m_Device->CreateQuery(&Desc, &m_TimestampEnd));

	Desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	HFALSE_IF_FAILED(m_Device->CreateQuery(&Desc, &m_DisjointQuery));

	return true;
}