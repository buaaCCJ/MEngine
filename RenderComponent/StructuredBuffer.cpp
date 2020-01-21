#include "StructuredBuffer.h"
#include "../Singleton/FrameResource.h"
StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	StructuredBufferElement* elementsArray,
	UINT elementsCount
) : MObject(), elements(elementsCount), offsets(elementsCount)
{
	memcpy(elements.data(), elementsArray, sizeof(StructuredBufferElement) * elementsCount);
	size_t offst = 0;
	for (UINT i = 0; i < elementsCount; ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mDefaultBuffer)
	));
}

void StructuredBuffer::TransformBufferState(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		mDefaultBuffer.Get(),
		beforeState,
		afterState
	));
}

size_t StructuredBuffer::GetStride(UINT index) const
{
	return elements[index].stride;
}
size_t StructuredBuffer::GetElementCount(UINT index) const
{
	return elements[index].elementCount;
}
D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::GetAddress(UINT element, UINT index) const
{

#ifdef NDEBUG
	auto& ele = elements[element];
	return mDefaultBuffer->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return mDefaultBuffer->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#endif
}

size_t StructuredBuffer::GetAddressOffset(UINT element, UINT index) const
{
#ifdef NDEBUG
	auto& ele = elements[element];
	return offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return offsets[element] + ele.stride * index;
#endif
}

void StructuredBuffer::ReleaseResourceAfterFlush(FrameResource* targetResource)
{
	FrameResource::ReleaseResourceAfterFlush(mDefaultBuffer, targetResource);
}