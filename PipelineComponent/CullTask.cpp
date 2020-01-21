#include "CullTask.h"
using namespace DirectX;
/*void CullTask::ScheduleCullingJob(std::vector<MeshRenderer*>* targets, DirectX::XMVECTOR* cullingPlanes, tf::Taskflow& flow)
{
	waitingBoundingBox = targets;
	memcpy(cameraCullingPlanes, cullingPlanes, sizeof(DirectX::XMVECTOR) * 6);
	cullingResult.clear();
	auto cullingFunc = [this]()->void
	{
		for (UINT i = 0; i < this->waitingBoundingBox->size(); ++i)
		{
			MeshRenderer* box = (*this->waitingBoundingBox)[i];
			XMMATRIX localToWorldMat = box->transform.GetLocalToWorldMatrix();
			for (UINT j = 0; j < box->mesh->GetSubmeshSize(); ++j)
			{
				SubMesh& sm = box->mesh->GetSubmesh(j);
				XMVECTOR position = XMLoadFloat3(&sm.boundingCenter);
				XMVECTOR extent = XMLoadFloat3(&sm.boundingExtent);
				if (MathLib::BoxIntersect(localToWorldMat, this->cameraCullingPlanes, position, extent))
					this->cullingResult.push_back({ i, j });
			}
		}
	};
	waitingTask = flow.emplace(std::move(cullingFunc));
}*/

std::vector<CullTask::CullingResult>* CullTask::GetCullingResult()
{
	return &cullingResult;
}
