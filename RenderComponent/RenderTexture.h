#pragma once
#include "../Common/MObject.h"
#include "../Common/d3dUtil.h"
#include "../RenderComponent/DescriptorHeap.h"
enum CubeMapFace
{
	CubeMapFace_PositiveX = 0,
	CubeMapFace_NegativeX = 1,
	CubeMapFace_PositiveY = 2,
	CubeMapFace_NegativeY = 3,
	CubeMapFace_PositiveZ = 4,
	CubeMapFace_NegativeZ = 5
};

enum RenderTextureDimension
{
	RenderTextureDimension_Tex2D = 0,
	RenderTextureDimension_Tex2DArray = 1,
	RenderTextureDimension_Tex3D = 2,
	RenderTextureDimension_Cubemap = 3
};
enum RenderTextureDepthSettings
{
	RenderTextureDepthSettings_None,
	RenderTextureDepthSettings_Depth16,
	RenderTextureDepthSettings_Depth32,
	RenderTextureDepthSettings_DepthStencil
};

enum class RenderTextureUsage : bool
{
	RenderTextureUsage_ColorBuffer = false,
	RenderTextureUsage_DepthBuffer = true
};

enum class RenderTextureState : bool
{
	Render_Target = false,
	Unordered_Access = true
};

struct RenderTextureFormat
{
	RenderTextureUsage usage;
	union {
		DXGI_FORMAT colorFormat;
		RenderTextureDepthSettings depthFormat;
	};
};

struct RenderTextureDescriptor
{
	UINT width;
	UINT height;
	UINT depthSlice;
	RenderTextureDimension type;
	RenderTextureFormat rtFormat;
	constexpr bool operator==(const RenderTextureDescriptor& other) const
	{
		bool value = width == other.width &&
			height == other.height &&
			depthSlice == other.depthSlice &&
			type == other.type &&
			rtFormat.usage == other.rtFormat.usage;
		if (value)
		{
			if (rtFormat.usage == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
			{
				return rtFormat.colorFormat == other.rtFormat.colorFormat;
			}
			else
			{
				return rtFormat.depthFormat == other.rtFormat.depthFormat;
			}
		}
		return false;
	}

	constexpr bool operator!=(const RenderTextureDescriptor& other) const
	{
		return !operator==(other);
	}
};

class RenderTexture : public MObject
{
private:
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	UINT depthSlice;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mipCount;
	RenderTextureUsage usage;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Microsoft::WRL::ComPtr<ID3D12Resource> mColorResource;
	DescriptorHeap rtvHeap;
	RenderTextureDimension mType;
	void GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel);
public:
	virtual ~RenderTexture();
	constexpr RenderTextureDimension GetType() const { return mType; }
	RenderTexture(
		ID3D12Device* device,
		UINT width,
		UINT height,
		RenderTextureFormat rtFormat,
		RenderTextureDimension type,
		UINT depthCount,
		UINT mipCount,
		RenderTextureState initState = RenderTextureState::Render_Target
	);
	constexpr UINT GetMipCount() { return mipCount; }
	void BindRTVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT slice);
	constexpr UINT GetWidth() { return mWidth; }
	constexpr UINT GetHeight() { return mHeight; }
	constexpr UINT GetDepthSlice() { return depthSlice; }
	void SetViewport(ID3D12GraphicsCommandList* commandList);
	ID3D12Resource* GetColorResource() const { return mColorResource.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetColorDescriptor(UINT slice);
	void BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
	void BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel);
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice);
	DXGI_FORMAT GetColorFormat() const { return mFormat; }
};