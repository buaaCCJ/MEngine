#include "PipelineComponent.h"
#include "../RenderComponent/RenderTexture.h"
#include "../PipelineComponent/IPerCameraResource.h"
#include "../Singleton/FrameResource.h"
#include "TempRTAllocator.h"
std::mutex PipelineComponent::mtx;
JobBucket* PipelineComponent::bucket(nullptr);
void PipelineComponent::ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator)
{
	for (auto ite = loadRTCommands.begin(); ite != loadRTCommands.end(); ++ite)
	{
		LoadTempRTCommand& cmd = *ite;
		MObject* obj = allocator->GetTempResource(device, cmd.uID, cmd.descriptor);
		allTempResource[cmd.index] = obj;
	}
	loadRTCommands.clear();
	for (auto ite = requiredRTs.begin(); ite != requiredRTs.end(); ++ite)
	{
		MObject* obj = allocator->GetUsingRenderTexture(ite->uID);
		allTempResource[ite->descIndex] = obj;
	}
	requiredRTs.clear();
	for (auto ite = unLoadRTCommands.begin(); ite != unLoadRTCommands.end(); ++ite)
	{
		allocator->ReleaseRenderTexutre(*ite);
	}
	unLoadRTCommands.clear();
}
/*
bool TemporalResourceCommand::operator==(const TemporalResourceCommand& other) const
{
	bool eq = type == other.type && uID == other.uID;
	if (type == TemporalResourceCommand::CommandType_Create_RenderTexture ||
		type == TemporalResourceCommand::CommandType_Create_StructuredBuffer)
	{
		return eq && descriptor.rtDesc == other.descriptor.rtDesc;
	}
	else
	{
		return eq;
	}
}*/

void PipelineComponent::CreateFence(ID3D12Device* device)
{
	if (fence == nullptr && GetCommandListType() != CommandListType_None)
	{
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED,
			IID_PPV_ARGS(&fence)));
	}
	dependingComponentCount++;
}

void PipelineComponent::ClearHandles()
{
	jobHandles.clear();
}
void PipelineComponent::MarkHandles()
{
	for (auto ite = cpuDepending.begin(); ite != cpuDepending.end(); ++ite)
	{
		auto& vec = (*ite)->jobHandles;
		for (auto selfIte = jobHandles.begin(); selfIte != jobHandles.end(); ++selfIte)
		{
			for (auto dependIte = vec.begin(); dependIte != vec.end(); ++dependIte)
			{
				dependIte->Precede(*selfIte);
			}
		}
	}
}
PipelineComponent::PipelineComponent()
{
	loadRTCommands.reserve(10);
	requiredRTs.reserve(10);
	unLoadRTCommands.reserve(10);
	jobHandles.reserve(10);
	gpuDepending.reserve(10);
	cpuDepending.reserve(10);
}