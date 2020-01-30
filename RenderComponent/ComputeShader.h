#pragma once
#include "../Common/d3dUtil.h"
#include "Shader.h"
class JobBucket;
struct ComputeShaderVariable
{
	enum Type
	{
		ConstantBuffer, StructuredBuffer, RWStructuredBuffer, SRVDescriptorHeap, UAVDescriptorHeap
	};
	std::string name;
	Type type;
	UINT tableSize;
	UINT registerPos;
	UINT space;
	ComputeShaderVariable() {}
	ComputeShaderVariable(
		const std::string& name,
		Type type,
		UINT tableSize,
		UINT registerPos,
		UINT space
	) : name(name),
		type(type),
		tableSize(tableSize),
		registerPos(registerPos),
		space(space)
	{}
};

class DescriptorHeap;
class StructuredBuffer;
class ComputeShaderCompiler;
class ComputeShader
{
	friend class ComputeShaderCompiler;
private:
	std::vector<Pass> allPasses;
	
	std::unordered_map<UINT, UINT> mVariablesDict;
	std::vector<ComputeShaderVariable> mVariablesVector;
	std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> csShaders;
	std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> mCommandSignature;
public:
	ComputeShader(
		std::wstring compilePath,
		std::string* kernelName, UINT kernelCount,
		ComputeShaderVariable* allShaderVariables, UINT varSize,
		ID3D12Device* device,
		JobBucket* compileJob);
	size_t VariableLength() const { return mVariablesVector.size(); }
	void BindRootSignature(ID3D12GraphicsCommandList* commandList);
	void BindRootSignature(ID3D12GraphicsCommandList* commandList, DescriptorHeap* heap);
	void SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset);
	void SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address);
	void Dispatch(ID3D12GraphicsCommandList* cList, UINT kernel, UINT x, UINT y, UINT z);
	void DispatchIndirect(ID3D12GraphicsCommandList* cList, UINT dispatchKernel, StructuredBuffer* indirectBuffer, UINT bufferElement, UINT bufferIndex);
	template<typename Func>
	void IterateVariables(const Func& f)
	{
		for (int i = 0; i < mVariablesVector.size(); ++i)
		{
			f(mVariablesVector[i]);
		}
	}
};