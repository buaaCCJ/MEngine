#include "MaterialManager.h"
#include "Shader.h"
#include "Material.h"
#include "../Singleton/FrameResource.h"
#include "UploadBuffer.h"
MaterialManager::MaterialManager(
	Shader* shader,
	UINT propertySize,
	ID3D12Device* device
) : mShader(shader), 
propertySize(propertySize),
elementPool(propertySize, propertySize > 256 ? 128 : 256),
pool(new UploadBuffer())
{
	pool->Create(device, elementPool.size(), false, propertySize);
	for (UINT i = 0, size = elementPool.size(); i < size; ++i)
	{
		elementPool[i] = i;
	}
}

UINT MaterialManager::GetElement(FrameResource* resource, ID3D12Device* device)
{
	if (elementPool.empty())
	{
		UINT lastSize = pool->GetElementCount();
		UINT newSize = lastSize * 2;
		for (UINT i = lastSize; i < newSize; ++i)
		{
			elementPool.push_back(i);
		}
		UploadBuffer* newBuffer = new UploadBuffer();
		newBuffer->Create(device, newSize, false, propertySize);
		newBuffer->CopyFrom(pool.get(), 0, 0, lastSize);
		pool->ReleaseAfterFlush(resource);
		pool = std::unique_ptr<UploadBuffer>(newBuffer);
	}
	auto ite = elementPool.end() - 1;
	UINT i = *ite;
	elementPool.erase(ite);
	return i;
}

void MaterialManager::ReturnElement(UINT ele)
{
	elementPool.push_back(ele);
}