#include "PSOContainer.h"
#include "MeshLayout.h"
PSOContainer::PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat):
	settings(1)
{
	PSORTSetting& set = settings[0];
	set.depthFormat = depthFormat;
	set.rtCount = rtCount;
	memcpy(set.rtFormat, allRTFormat, sizeof(DXGI_FORMAT) * rtCount);
	allPSOState.reserve(50);
}
PSOContainer::PSOContainer(PSORTSetting* formats, uint formatCount) : 
	settings(formatCount)
{
	memcpy(settings.data(), formats, sizeof(PSORTSetting) * formatCount);
	allPSOState.reserve(50);
}
bool PSODescriptor::operator==(const PSODescriptor& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}

bool PSODescriptor::operator==(const PSODescriptor&& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}
ID3D12PipelineState* PSOContainer::GetState(PSODescriptor& desc, ID3D12Device* device, uint index)
{
	desc.GenerateHash();
	PSORTSetting& set = settings[index];
	auto&& ite = allPSOState.find(std::pair<uint, PSODescriptor>(index, desc));
	if (ite == allPSOState.end())
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		std::vector<D3D12_INPUT_ELEMENT_DESC>* inputElement = MeshLayout::GetMeshLayoutValue(desc.meshLayoutIndex);
		opaquePsoDesc.InputLayout = { inputElement->data(), (UINT)inputElement->size() };
		desc.shaderPtr->GetPassPSODesc(desc.shaderPass, &opaquePsoDesc);
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		opaquePsoDesc.NumRenderTargets = set.rtCount;
		memcpy(&opaquePsoDesc.RTVFormats, set.rtFormat, set.rtCount * sizeof(DXGI_FORMAT));
		opaquePsoDesc.SampleDesc.Count = 1;
		opaquePsoDesc.SampleDesc.Quality = 0;
		opaquePsoDesc.DSVFormat = set.depthFormat;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> result = nullptr;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(result.GetAddressOf())));
		allPSOState.insert_or_assign(std::pair<uint, PSODescriptor>(index, desc), result);
		return result.Get();
	};
	return ite->second.Get();
	
}