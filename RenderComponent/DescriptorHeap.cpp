#pragma once
#include "DescriptorHeap.h"
DescriptorHeap::DescriptorHeap(
	ID3D12Device* pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT NumDescriptors,
	bool bShaderVisible)
{
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	Desc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&Desc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&pDH));
	hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
	hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);

}

void DescriptorHeap::Create(ID3D12Device* pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT NumDescriptors,
	bool bShaderVisible)
{
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	Desc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&Desc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&pDH));
	hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
	hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
}