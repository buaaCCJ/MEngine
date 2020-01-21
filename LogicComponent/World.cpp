#include "World.h"
#include "../Common/GeometryGenerator.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/DescriptorHeap.h"
using namespace DirectX;
World* World::current = nullptr;
World::World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device) :
	usedDescs(MAXIMUM_HEAP_COUNT),
	unusedDescs(MAXIMUM_HEAP_COUNT),
	globalDescriptorHeap(new DescriptorHeap)
{
	globalDescriptorHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAXIMUM_HEAP_COUNT, true);
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
}

UINT World::GetDescHeapIndexFromPool()
{
	std::lock_guard lck(mtx);
	auto&& last = unusedDescs.end() - 1;
	UINT value = *last;
	unusedDescs.erase(last);
	usedDescs[value] = true;
	return value;
}

World::~World()
{
	globalDescriptorHeap.Destroy();
}

void World::ReturnDescHeapIndexToPool(UINT target)
{
	std::lock_guard lck(mtx);
	auto ite = usedDescs[target];
	if (ite)
	{
		unusedDescs.push_back(target);
		ite = false;
	}
}

void World::ForceCollectAllHeapIndex()
{
	std::lock_guard lck(mtx);
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
	usedDescs.Clear();
}

void World::Update(FrameResource* resource)
{

}