#pragma once
#include "TempRTAllocator.h"
#include "../Common/d3dUtil.h"
#include <unordered_map>
#include "../Common/MetaLib.h"
#include "../JobSystem/JobSystem.h"
#include "CommandBuffer.h"
class FrameResource;
class Camera;
class World;
class PipelineComponent;
struct RenderPipelineData
{
	ID3D12Device* device;
	ID3D12Resource* backBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	FrameResource* lastResource;
	FrameResource* resource;
	std::vector<Camera*>* allCameras;
	ID3D12Fence* fence;
	UINT64* fenceIndex;
	bool executeLastFrame;
	World* world;
	float time;
	float deltaTime;
};
class RenderPipeline final
{
private:
	static RenderPipeline* current;
	struct RenderTextureMark
	{
		UINT id;
		UINT rtIndex;
		ResourceDescriptor desc;
		UINT startComponent;
		UINT endComponent;
	};
	UINT initCount = 0;
	std::vector<PipelineComponent*> components;
	TempRTAllocator tempRTAllocator;
	static std::unordered_map<std::string, PipelineComponent*> componentsLink;
	std::vector<std::vector<PipelineComponent*>> renderPathComponents;
	Dictionary<UINT, RenderTextureMark> renderTextureMarks;
	template<typename T>
	void Init()
	{
		T* ptr = new T();
		components.emplace_back(ptr);
		componentsLink.insert_or_assign(typeid(T).name(), ptr);
	}
	RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
public:
	static RenderPipeline* GetInstance(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void DestroyInstance();
	~RenderPipeline();
#ifdef NDEBUG
	void PrepareRendering(RenderPipelineData& data, JobSystem* jobSys, std::vector <JobBucket*>& bucketArray) noexcept;
	void ExecuteRendering(RenderPipelineData& data) noexcept;
#else
	void PrepareRendering(RenderPipelineData& data, JobSystem* jobSys, std::vector <JobBucket*>& bucketArray);
	void ExecuteRendering(RenderPipelineData& data);
#endif
	template <typename T>
	static T* GetComponent()
	{
		std::string str(typeid(T).name());
		auto&& ite = componentsLink.find(str);
		if (ite != componentsLink.end())
			return (T*)ite->second;
		else return nullptr;
	}
};