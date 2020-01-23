#include "Shader.h"
#include "../Singleton/ShaderID.h"
#include "Texture.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "UploadBuffer.h"
#include <fstream>
#include "../JobSystem/JobInclude.h"
using namespace std;
using Microsoft::WRL::ComPtr;
Shader::~Shader()
{

}

void Shader::BindRootSignature(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
}

void Shader::BindRootSignature(ID3D12GraphicsCommandList* commandList, DescriptorHeap* descHeap)
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
	if (descHeap)
		descHeap->SetDescriptorHeap(commandList);
}

void Shader::GetPassPSODesc(UINT pass, D3D12_GRAPHICS_PIPELINE_STATE_DESC* targetPSO)
{
	Pass& p = allPasses[pass];
	targetPSO->VS =
	{
		reinterpret_cast<BYTE*>(p.vsShader->GetBufferPointer()),
		p.vsShader->GetBufferSize()
	};
	targetPSO->PS =
	{
		reinterpret_cast<BYTE*>(p.psShader->GetBufferPointer()),
		p.psShader->GetBufferSize()
	};
	targetPSO->BlendState = p.blendState;
	targetPSO->RasterizerState = p.rasterizeState;
	targetPSO->pRootSignature = mRootSignature.Get();
	targetPSO->DepthStencilState = p.depthStencilState;
}

Shader::Shader(
	Pass* passes, UINT passCount,
	ShaderVariable* shaderVariables, UINT shaderVarCount,
	ID3D12Device* device,
	JobBucket* compileJob
)
{
	//Create Pass
	allPasses.reserve(passCount);
	for (int i = 0; i < passCount; ++i)
	{
		Pass& p = passes[i];
		allPasses.push_back(std::move(p));
	}
	std::vector<Pass>* allPassesPtr = &allPasses;
	auto func = [passCount, allPassesPtr]()->void
	{
		for (int i = 0; i < passCount; ++i)
		{
			Pass& p = (*allPassesPtr)[i];
			if (p.vsShader == nullptr)
				p.vsShader = d3dUtil::CompileShader(p.filePath, nullptr, p.vertex, "vs_5_1");//CompileShader(p.filePath, p.vertex, "vs_5_1", nullptr, useShaderCache);
			if (p.psShader == nullptr)
				p.psShader = d3dUtil::CompileShader(p.filePath, nullptr, p.fragment, "ps_5_1");// CompileShader(p.filePath, p.fragment, "ps_5_1", nullptr, useShaderCache);
		}
	};
	if (compileJob)
	{
		compileJob->GetTask(func);
	}
	else {
		func();
	}
	
	mVariablesDict.reserve(shaderVarCount + 2);
	mVariablesVector.reserve(shaderVarCount);
	for (int i = 0; i < shaderVarCount; ++i)
	{
		ShaderVariable& variable = shaderVariables[i];
		mVariablesDict[ShaderID::PropertyToID(variable.name)] = i;
		mVariablesVector.push_back(variable);

	}

	vector<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	std::vector< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	IterateVariables([&](ShaderVariable& var) -> void {
		if (var.type == ShaderVariableType_DescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
	});
	UINT offset = 0;
	IterateVariables([&](ShaderVariable& var) -> void
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter;
		switch (var.type)
		{
		case ShaderVariableType_DescriptorHeap:
			slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
			offset++;
			break;
		case ShaderVariableType_ConstantBuffer:
			slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
			break;
		case ShaderVariableType_StructuredBuffer:
			slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
			break;
		default:
			return;
		}
		allParameter.push_back(slotRootParameter);
	});
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(allParameter.size(), allParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

int Shader::GetPropertyRootSigPos(UINT id)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return -1;
	return (int)ite->second;
}

void Shader::SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset)
{
	if (targetObj == nullptr) return;
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	ShaderVariable& var = mVariablesVector[rootSigPos];
	UploadBuffer* uploadBufferPtr;
	ID3D12DescriptorHeap* heap = nullptr;
	switch (var.type)
	{
	case ShaderVariableType_DescriptorHeap:
		heap = ((DescriptorHeap*)targetObj)->Get().Get();
		commandList->SetGraphicsRootDescriptorTable(
			rootSigPos,
			((DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ShaderVariableType_ConstantBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetGraphicsRootConstantBufferView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetAlignedStride()
		);
		break;
	case ShaderVariableType_StructuredBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetGraphicsRootShaderResourceView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetStride());
		break;
	}
}

ShaderVariable Shader::GetVariable(std::string name)
{
	return mVariablesVector[mVariablesDict[ShaderID::PropertyToID(name)]];
}

ShaderVariable Shader::GetVariable(UINT id)
{
	return mVariablesVector[mVariablesDict[id]];
}

bool Shader::TryGetShaderVariable(UINT id, ShaderVariable& targetVar)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return false;
	targetVar = mVariablesVector[ite->second];
	return true;
}

void Shader::SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	commandList->SetGraphicsRootShaderResourceView(
		rootSigPos,
		address);
}
