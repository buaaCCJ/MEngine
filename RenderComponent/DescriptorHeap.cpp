#pragma once
#include "DescriptorHeap.h"
HRESULT DescriptorHeap::Create(
	ID3D12Device* pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT NumDescriptors,
	bool bShaderVisible)
{
	mNumDescriptors = NumDescriptors;
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	Desc.NodeMask = 0;
	HRESULT hr = pDevice->CreateDescriptorHeap(&Desc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&pDH);
	if (FAILED(hr)) return hr;

	hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
	hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
	return hr;
}