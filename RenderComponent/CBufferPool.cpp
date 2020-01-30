#include "CBufferPool.h" 
#include <math.h>
#include "UploadBuffer.h"

void CBufferPool::Add(ID3D12Device* device)
{
	UploadBuffer* tPtr = new UploadBuffer(device, capacity, true, stride);
	arr.push_back(tPtr);
	for (UINT i = 0; i < capacity; ++i)
	{
		poolValue.push_back({ tPtr, i });
	}
}

CBufferPool::CBufferPool(UINT stride, UINT initCapacity) :
	capacity(initCapacity),
	stride(stride)
{
	poolValue.reserve(initCapacity);
	arr.reserve(10);
}

CBufferPool::~CBufferPool()
{
	for (auto ite = arr.begin(); ite != arr.end(); ++ite)
	{
		delete *ite;
	}
}

ConstBufferElement CBufferPool::Get(ID3D12Device* device)
{
	UINT value = capacity;
	if (poolValue.empty())
	{
		Add(device);
	}
	auto&& ite = poolValue.end() - 1;
	ConstBufferElement pa = *ite;
	poolValue.erase(ite);
	return pa;

}

void CBufferPool::Return(ConstBufferElement& target)
{
	poolValue.push_back(target);
}