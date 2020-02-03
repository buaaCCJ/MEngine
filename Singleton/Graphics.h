#pragma once
#include "../Common/d3dUtil.h"
class PSOContainer;
class Shader;
class RenderTexture;
class UploadBuffer;
enum BackBufferState
{
	BackBufferState_Present = 0,
	BackBufferState_RenderTarget = 1
};

enum CopyTarget
{
	CopyTarget_DepthBuffer = 0,
	CopyTarget_ColorBuffer = 1
};

class Graphics
{
public:
	static void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void Blit(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
		UINT renderTargetCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
		PSOContainer* container, uint containerIndex,
		UINT width, UINT height,
		Shader* shader, UINT pass);

	inline static void ResourceStateTransform(
		ID3D12GraphicsCommandList* commandList,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState,
		ID3D12Resource* resource)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			beforeState,
			afterState
		));
	}

	static void CopyTexture(
		ID3D12GraphicsCommandList* commandList,
		RenderTexture* source, CopyTarget sourceTarget, UINT sourceSlice, UINT sourceMipLevel,
		RenderTexture* dest, CopyTarget destTarget, UINT destSlice, UINT destMipLevel);

	static void CopyBufferToTexture(
		ID3D12GraphicsCommandList* commandList,
		UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
		ID3D12Resource* textureResource, UINT targetMip,
		UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize);
};