#pragma once

#ifndef POSTPROCESSMANAGER_H
#define POSTPROCESSMANAGER_H

#include <memory>
#include <vector>

class Profiler;
class CameraManager;
class IPostProcess;

class PostProcessManager
{
public:
	~PostProcessManager();

	bool Init(std::shared_ptr<Profiler> Profiler, std::shared_ptr<CameraManager> CameraManager);
	void Shutdown();

	bool IsUsingTAA() const;

	std::vector<std::unique_ptr<IPostProcess>>& GetPostProcesses() { return m_PostProcesses; }

private:
	std::vector<std::unique_ptr<IPostProcess>> m_PostProcesses;

	IPostProcess* m_pTAA = nullptr;

};

#endif
