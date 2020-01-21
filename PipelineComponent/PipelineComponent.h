#pragma once
#include "../Common/Camera.h"
#include "../JobSystem/JobInclude.h"
#include "RenderPipeline.h"
class ThreadCommand;
class FrameResource;
class TempRTAllocator;
class RenderTexture;
class World;

struct TemporalResourceCommand
{
	enum CommandType
	{
		CommandType_Create_RenderTexture,
		CommandType_Require_RenderTexture,
		CommandType_Create_StructuredBuffer,
		CommandType_Require_StructuredBuffer
	};
	CommandType type;
	UINT uID;
	ResourceDescriptor descriptor;


	//bool operator==(const TemporalResourceCommand& other) const;
};
class PerCameraRenderingEvent;
class CommandBuffer;
enum CommandListType
{
	CommandListType_None = 0,
	CommandListType_Graphics = 1,
	CommandListType_Compute = 2
};
struct RequiredRT
{
	UINT descIndex;
	UINT uID;
};
class PipelineComponent
{
	friend class RenderPipeline;
	friend class PerCameraRenderingEvent;
private:
	static std::mutex mtx;
	static JobBucket* bucket;
	ThreadCommand* threadCommand;//thread command cache
	struct LoadTempRTCommand
	{
		UINT uID;
		UINT index;
		ResourceDescriptor descriptor;
	};
	std::vector<JobHandle> jobHandles;
	std::vector<LoadTempRTCommand> loadRTCommands;
	std::vector<UINT> unLoadRTCommands;
	std::vector<PipelineComponent*> cpuDepending;
	std::vector<PipelineComponent*> gpuDepending;
	std::vector<RequiredRT> requiredRTs;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	UINT dependingComponentCount = 0;
	void CreateFence(ID3D12Device* device);
	void ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator);
	template <typename ... T>
	class Depending;

	template <>
	class Depending<>
	{
	public:
		Depending(std::vector<PipelineComponent*>&) {}
	};

	template <typename T, typename ... Args>
	class Depending<T, Args...>
	{
	public:
		Depending(std::vector<PipelineComponent*>& vec)
		{
			vec.push_back(RenderPipeline::GetComponent<T>());
			Depending<Args...> d(vec);
		}
	};

protected:
	std::vector<MObject*> allTempResource;
	template <typename... Args>
	void SetCPUDepending()
	{
		cpuDepending.clear();
		cpuDepending.reserve(sizeof...(Args));
		Depending<Args...> d(cpuDepending);
		
	}
	template <typename... Args>
	void SetGPUDepending()
	{
		gpuDepending.clear();
		gpuDepending.reserve(sizeof...(Args));
		Depending<Args...> d(gpuDepending);
	}
public:

	struct EventData
	{
		ID3D12Device* device;
		ID3D12Resource* backBuffer;
		Camera* camera;
		FrameResource* resource;
		World* world;
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
		UINT width, height;
		float deltaTime;
		float time;
		bool isBackBufferForPresent;
	};
	template <typename Func>
	JobHandle ScheduleJob(const Func& func)
	{
		JobHandle handle = bucket->GetTask(func);
		jobHandles.push_back(handle);
		return handle;
	}

	virtual CommandListType GetCommandListType() = 0;
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void Dispose() = 0;
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt) = 0;
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList) = 0;
	void ClearHandles();
	void MarkHandles();
	~PipelineComponent() {}
	PipelineComponent();
};