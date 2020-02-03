#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
#include <atomic>
struct StateTransformBuffer
{
	ID3D12Resource* targetResource;
	D3D12_RESOURCE_STATES beforeState;
	D3D12_RESOURCE_STATES afterState;
};
class PipelineComponent;
class StructuredBuffer;
class RenderTexture;
enum ResourceReadWriteState : bool
{
	Write = false,
	Read = true
};
class ThreadCommand final
{
	friend class PipelineComponent;
private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
	std::unordered_map<RenderTexture*, ResourceReadWriteState> rtStateMap;
	std::unordered_map<StructuredBuffer*, ResourceReadWriteState> sbufferStateMap;
public:
	inline ID3D12CommandAllocator* GetAllocator() const { return cmdAllocator.Get(); }
	inline ID3D12GraphicsCommandList* GetCmdList() const { return cmdList.Get(); }
	ThreadCommand(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
	void SetResourceReadWriteState(RenderTexture* rt, ResourceReadWriteState state);
	void SetResourceReadWriteState(StructuredBuffer* rt, ResourceReadWriteState state);
	void ResetCommand();
	void CloseCommand();
};