#pragma once
#include "../RenderComponent/MeshRenderer.h"
#include "../Singleton/MathLib.h"

class CullTask
{
public:
	struct CullingResult
	{
		UINT rendererIndex;
		UINT submeshIndex;
	};
private:
	std::vector<MeshRenderer*>* waitingBoundingBox;
	std::vector<CullingResult> cullingResult;
	DirectX::XMVECTOR cameraCullingPlanes[6];
	//JobHandler waitingHandler;
	//tf::Task waitingTask;
public:
	std::vector<CullingResult>* GetCullingResult();
	//tf::Task& GetTask() { return waitingTask; }
	/*void ScheduleCullingJob(
		std::vector<MeshRenderer*>* targets,
		DirectX::XMVECTOR* cullingPlanes,
		tf::Taskflow& flow);*/
};
