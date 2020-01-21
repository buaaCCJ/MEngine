#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/BitArray.h"
#include <mutex>
class FrameResource;
class Transform;
class DescriptorHeap;
//Only For Test!
class World final
{
private:
	static const UINT MAXIMUM_HEAP_COUNT = 10000;
	ObjectPtr<DescriptorHeap> globalDescriptorHeap;
	BitArray usedDescs;
	std::vector<UINT> unusedDescs;
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	static World* current;
	std::mutex mtx;
	
public:
	std::vector<Transform*> allTransformsPtr;
	~World();
	UINT windowWidth;
	UINT windowHeight;
	static constexpr World* GetInstance() { return current; }
	static constexpr World* CreateInstance(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
	{
		if (current)
			return current;
		current = new World(cmdList, device);
		return current;
	}
	static constexpr void DestroyInstance()
	{
		if (current) delete current;
		current = nullptr;
	}
	void Update(FrameResource* resource);
	constexpr DescriptorHeap* GetGlobalDescHeap() const
	{ return globalDescriptorHeap.operator->(); }
	UINT GetDescHeapIndexFromPool();
	void ReturnDescHeapIndexToPool(UINT targetIndex);
	void ForceCollectAllHeapIndex();
};