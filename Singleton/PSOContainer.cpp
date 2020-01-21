#include "PSOContainer.h"
#include "MeshLayout.h"
PSOContainer::PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat):
	depthFormat(depthFormat),
	rtCount(rtCount)
{
	memcpy(rtFormat, allRTFormat, sizeof(DXGI_FORMAT) * rtCount);
	allPSOState.reserve(200);
}
bool PSODescriptor::operator==(const PSODescriptor& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}

bool PSODescriptor::operator==(const PSODescriptor&& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}
ID3D12PipelineState* PSOContainer::GetState(PSODescriptor& desc, ID3D12Device* device)
{
	desc.GenerateHash();
	auto&& ite = allPSOState.find(desc);
	if (ite == allPSOState.end())
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		std::vector<D3D12_INPUT_ELEMENT_DESC>* inputElement = MeshLayout::GetMeshLayoutValue(desc.meshLayoutIndex);
		opaquePsoDesc.InputLayout = { inputElement->data(), (UINT)inputElement->size() };
		desc.shaderPtr->GetPassPSODesc(desc.shaderPass, &opaquePsoDesc);
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		opaquePsoDesc.NumRenderTargets = rtCount;
		memcpy(&opaquePsoDesc.RTVFormats, rtFormat, rtCount * sizeof(DXGI_FORMAT));
		opaquePsoDesc.SampleDesc.Count = 1;
		opaquePsoDesc.SampleDesc.Quality = 0;
		opaquePsoDesc.DSVFormat = depthFormat;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> result = nullptr;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(result.GetAddressOf())));
		allPSOState.insert_or_assign(desc, result);
		return result.Get();
	};
	return ite->second.Get();
	
}