#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../RenderComponent/CBufferPool.h"
#include <functional>
#include "../PipelineComponent/ThreadCommand.h"
#include "../Common/Pool.h"
#include "../JobSystem/JobSystem.h"
#include "../PipelineComponent/IPerCameraResource.h"
#include <mutex>
class CommandBuffer;
class Camera;
class PipelineComponent;
struct Vertex
{
    float3 Pos;
    float3 Normal;
	float2 TexC;
};


struct ObjectConstants
{
	float4x4 lastObjectToWorld = MathHelper::Identity4x4();
	float4x4 objectToWorld = MathHelper::Identity4x4();
	uint2 id = { 0,0 };
};

struct PassConstants
{
	float4x4 View = MathHelper::Identity4x4();
	float4x4 InvView = MathHelper::Identity4x4();
	float4x4 Proj = MathHelper::Identity4x4();
	float4x4 InvProj = MathHelper::Identity4x4();
	float4x4 ViewProj = MathHelper::Identity4x4();
	float4x4 InvViewProj = MathHelper::Identity4x4();
	float4x4 nonJitterVP = MathHelper::Identity4x4();
	float4x4 nonJitterInverseVP = MathHelper::Identity4x4();
	float4x4 lastVP = MathHelper::Identity4x4();
	float4x4 lastInverseVP = MathHelper::Identity4x4();
	float3 worldSpaceCameraPos;
	float NearZ = 0.0f;
	float FarZ = 0.0f;
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct FrameResource
{
private:
	struct FrameResCamera
	{
		std::unordered_map<void*, IPipelineResource*> perCameraResource;
		std::vector<ThreadCommand*> graphicsThreadCommands;
		std::vector<ThreadCommand*> computeThreadCommands;
		FrameResCamera()
		{
			perCameraResource.reserve(50);
			graphicsThreadCommands.reserve(20);
			computeThreadCommands.reserve(20);
		}
		~FrameResCamera()
		{
			for (auto ite = perCameraResource.begin(); ite != perCameraResource.end(); ++ite)
			{
				delete ite->second;
			}
		}
	};
	static Pool<ThreadCommand> threadCommandMemoryPool;
	static Pool<FrameResCamera> perCameraDataMemPool;
	static std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> needClearResourcesAfterFlush;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> needClearResources;
	std::unordered_map<Camera*, FrameResCamera*> perCameraDatas;
	std::unordered_map<void*, IPipelineResource*> perFrameResource;
	std::mutex mtx;
public:
	ThreadCommand* commmonThreadCommand;
	static CBufferPool cameraCBufferPool;
	static std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	static FrameResource* mCurrFrameResource;
	std::unique_ptr<CommandBuffer> commandBuffer = nullptr;
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();
	void UpdateBeforeFrame(ID3D12Fence* mFence);
	static void ReleaseResourceAfterFlush(Microsoft::WRL::ComPtr<ID3D12Resource>& resources, FrameResource* resource);
	void UpdateAfterFrame(UINT64& currentFence, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence);
	void WaitForLastFrameResource(UINT64& fenceCount, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence);
	ThreadCommand* GetNewThreadCommand(Camera* cam, ID3D12Device* device, D3D12_COMMAND_LIST_TYPE cmdListType);
	void ReleaseThreadCommand(Camera* cam, ThreadCommand* targetCmd, D3D12_COMMAND_LIST_TYPE cmdListType);
	void OnLoadCamera(Camera* targetCamera, ID3D12Device* device);
	void OnUnloadCamera(Camera* targetCamera);
    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    //Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
   // std::unique_ptr<UploadBuffer<FrameConstants>> FrameCB = nullptr;
	std::unordered_map<UINT, ConstBufferElement> cameraCBs;
	std::unordered_map<UINT, ConstBufferElement> objectCBs;
    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
	//Rendering Events
	template <typename Func>
	inline IPipelineResource* GetPerCameraResource(void* targetComponent, Camera* cam, const Func& func)
	{
		std::lock_guard<std::mutex> lck(mtx);
		FrameResCamera* ptr = perCameraDatas[cam];
		auto&& ite = ptr->perCameraResource.find(targetComponent);
		if (ite == ptr->perCameraResource.end())
		{
			IPipelineResource* newComp = func();
			ptr->perCameraResource.insert_or_assign(targetComponent, newComp);
			return newComp;
		}
		return ite->second;
	}
	template <typename Func>
	inline IPipelineResource* GetResource(void* targetComponent, const Func& func)
	{
		std::lock_guard<std::mutex> lck(mtx);
		auto&& ite = perFrameResource.find(targetComponent);
		if (ite == perFrameResource.end())
		{
			IPipelineResource* newComp = func();
			perFrameResource.insert_or_assign(targetComponent, newComp);
			return newComp;
		}
		return ite->second;
	}

	void DisposeResource(void* targetComponent)
	{
		std::lock_guard<std::mutex> lck(mtx);
		auto&& ite = perFrameResource.find(targetComponent);
		if (ite == perFrameResource.end()) return;
		delete ite->second;
		perFrameResource.erase(ite);
	}

	void DisposeResource(void* targetComponent, Camera* cam)
	{
		std::lock_guard<std::mutex> lck(mtx);
		FrameResCamera* ptr = perCameraDatas[cam];
		auto&& ite = ptr->perCameraResource.find(targetComponent);
		if (ite == ptr->perCameraResource.end()) return;
		delete ite->second;
		ptr->perCameraResource.erase(ite);
	}
};