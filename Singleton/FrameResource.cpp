#include "../PipelineComponent/CommandBuffer.h"
#include "FrameResource.h"
#include "../Common/Camera.h"
std::vector<std::unique_ptr<FrameResource>> FrameResource::mFrameResources;
Pool<ThreadCommand> FrameResource::threadCommandMemoryPool(100);
Pool<FrameResource::FrameResCamera> FrameResource::perCameraDataMemPool(50);
std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> FrameResource::needClearResourcesAfterFlush;
FrameResource* FrameResource::mCurrFrameResource = nullptr;
CBufferPool FrameResource::cameraCBufferPool(sizeof(PassConstants), 256);
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
	perCameraDatas.reserve(20);
	cameraCBs.reserve(50);
	objectCBs.reserve(50);
	needClearResources.reserve(10);
	needClearResourcesAfterFlush.reserve(10);
	commmonThreadCommand = new ThreadCommand(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void FrameResource::UpdateBeforeFrame(ID3D12Fence* mFence)
{
	if (Fence != 0)
	{
		if (mFence->GetCompletedValue() < Fence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			ThrowIfFailed(mFence->SetEventOnCompletion(Fence, eventHandle));
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		needClearResources.clear();
		needClearResourcesAfterFlush.clear();
	}
}

void FrameResource::UpdateAfterFrame(UINT64& currentFence, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence)
{
	// Advance the fence value to mark commands up to this fence point.
	Fence = ++currentFence;

	commandQueue->Signal(mFence, Fence);

}

void FrameResource::WaitForLastFrameResource(UINT64& fenceCount, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence)
{
	commandQueue->Wait(mFence, fenceCount);
}

void FrameResource::OnLoadCamera(Camera* targetCamera, ID3D12Device* device)
{
	perCameraDatas[targetCamera] = perCameraDataMemPool.New();
	ConstBufferElement constBuffer = cameraCBufferPool.Get(device);
	cameraCBs[targetCamera->GetInstanceID()] = constBuffer;
}
void FrameResource::OnUnloadCamera(Camera* targetCamera)
{
	FrameResCamera* data = perCameraDatas[targetCamera];
	for (auto ite = data->graphicsThreadCommands.begin(); ite != data->graphicsThreadCommands.end(); ++ite)
	{
		threadCommandMemoryPool.Delete(*ite);
	}
	for (auto ite = data->computeThreadCommands.begin(); ite != data->computeThreadCommands.end(); ++ite)
	{
		threadCommandMemoryPool.Delete(*ite);
	}
	perCameraDatas.erase(targetCamera);
	ConstBufferElement& constBuffer = cameraCBs[targetCamera->GetInstanceID()];
	cameraCBufferPool.Return(constBuffer);
	cameraCBs.erase(targetCamera->GetInstanceID());
	perCameraDataMemPool.Delete(data);
}

void FrameResource::ReleaseResourceAfterFlush(Microsoft::WRL::ComPtr<ID3D12Resource>& resources, FrameResource* resource)
{
	if (resource == nullptr)
		needClearResourcesAfterFlush.push_back(resources);
	else
	{
		std::lock_guard lck(resource->mtx);
		resource->needClearResources.push_back(resources);
	}
}


ThreadCommand* FrameResource::GetNewThreadCommand(Camera* cam, ID3D12Device* device, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	std::vector<ThreadCommand*>* threadCommands = nullptr;
	switch (cmdListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		threadCommands = &perCameraDatas[cam]->graphicsThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		threadCommands = &perCameraDatas[cam]->computeThreadCommands;
		break;
	}
	if (threadCommands->empty())
	{
		return threadCommandMemoryPool.New(device, cmdListType);
	}
	else
	{
		auto&& ite = threadCommands->end() - 1;
		ThreadCommand* result = *ite;
		threadCommands->erase(ite);
		return result;
	}
}
void FrameResource::ReleaseThreadCommand(Camera* cam, ThreadCommand* targetCmd, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	std::vector<ThreadCommand*>* threadCommands = nullptr;
	switch (cmdListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		threadCommands = &perCameraDatas[cam]->graphicsThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		threadCommands = &perCameraDatas[cam]->computeThreadCommands;
		break;
	}
	threadCommands->push_back(targetCmd);
}

FrameResource::~FrameResource()
{
	if (mCurrFrameResource == this)
		mCurrFrameResource = nullptr;
	for (auto ite = perFrameResource.begin(); ite != perFrameResource.end(); ++ite)
	{
		delete ite->second;
	}
	delete commmonThreadCommand;
}