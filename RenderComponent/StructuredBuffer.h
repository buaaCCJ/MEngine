#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class FrameResource;
struct StructuredBufferElement
{
	size_t stride;
	size_t elementCount;
};

enum StructuredBufferState
{
	StructuredBufferState_UAV = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	StructuredBufferState_Indirect = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
	StructuredBufferState_Read = D3D12_RESOURCE_STATE_GENERIC_READ
};

class StructuredBuffer : public MObject
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mDefaultBuffer;
	std::vector<StructuredBufferElement> elements;
	std::vector<size_t> offsets;
public:
	StructuredBuffer(
		ID3D12Device* device,
		StructuredBufferElement* elementsArray,
		UINT elementsCount
	);
	size_t GetStride(UINT index) const;
	size_t GetElementCount(UINT index) const;
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT element, UINT index) const;
	size_t GetAddressOffset(UINT element, UINT index) const;
	inline ID3D12Resource* GetResource() const { return mDefaultBuffer.Get(); }
	void ReleaseResourceAfterFlush(FrameResource* targetResource);
	void TransformBufferState(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
};

class StructuredBufferStateBarrier final
{
	StructuredBuffer* sbuffer;
	D3D12_RESOURCE_STATES beforeState;
	D3D12_RESOURCE_STATES afterState;
	ID3D12GraphicsCommandList* commandList;
public:
	StructuredBufferStateBarrier(ID3D12GraphicsCommandList* commandList, StructuredBuffer* sbuffer, StructuredBufferState beforeState, StructuredBufferState afterState) :
		beforeState((D3D12_RESOURCE_STATES)beforeState), afterState((D3D12_RESOURCE_STATES)afterState), sbuffer(sbuffer),
		commandList(commandList)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			sbuffer->GetResource(),
			this->beforeState,
			this->afterState
		));
	}
	~StructuredBufferStateBarrier()
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			sbuffer->GetResource(),
			afterState,
			beforeState
		));
	}
};