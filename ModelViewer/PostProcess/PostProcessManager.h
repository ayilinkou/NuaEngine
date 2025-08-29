#pragma once

#ifndef POSTPROCESSMANAGER_H
#define POSTPROCESSMANAGER_H

#include <memory>
#include <vector>

#include "PostProcessSSAO.h"

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

	PostProcessSSAO* GetSSAO() const { return m_pSSAO.get(); }

private:
	std::vector<std::unique_ptr<IPostProcess>> m_PostProcesses;

	IPostProcess* m_pTAA = nullptr;
	std::unique_ptr<PostProcessSSAO> m_pSSAO;
};

#endif
