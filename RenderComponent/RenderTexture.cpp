#include "RenderTexture.h"
RenderTexture::~RenderTexture()
{
	mColorResource = nullptr;

}

void RenderTexture::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice)
{
#ifndef NDEBUG
	if (mType == RenderTextureDimension_Tex3D)
	{
		throw "Tex3D can not be cleaned!";
	}
#endif
	if (usage == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
	{
		float colors[4] = { 0,0,0,0 };
		commandList->ClearRenderTargetView(rtvHeap.hCPU(slice), colors, 0, nullptr);
	}
	else
		commandList->ClearDepthStencilView(rtvHeap.hCPU(slice), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0, 0, 0, nullptr);
}
void RenderTexture::SetViewport(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
}

void RenderTexture::GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel)
{
	UINT maxLevel = mColorResource->GetDesc().MipLevels - 1;
	targetMipLevel = min(targetMipLevel, maxLevel);
	uavDesc.Format = mColorResource->GetDesc().Format;
	switch (mType)
	{
	case RenderTextureDimension::RenderTextureDimension_Tex2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = targetMipLevel;
		uavDesc.Texture2D.PlaneSlice = 0;
		break;
	case RenderTextureDimension::RenderTextureDimension_Tex3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.MipSlice = targetMipLevel;
		uavDesc.Texture3D.WSize = depthSlice;
		break;
	default:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = depthSlice;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = targetMipLevel;
		uavDesc.Texture2DArray.PlaneSlice = 0;
		break;
	}
}

void RenderTexture::GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mColorResource->GetDesc().Format;
	switch (mType)
	{
	case RenderTextureDimension::RenderTextureDimension_Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureDimension::RenderTextureDimension_Tex2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureDimension::RenderTextureDimension_Tex2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.ArraySize = depthSlice;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		break;
	case RenderTextureDimension::RenderTextureDimension_Tex3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	}

}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetColorDescriptor(UINT slice)
{
#ifndef NDEBUG
	if (mType == RenderTextureDimension_Tex3D)
	{
		throw "Tex3D have no rtv heap!";
	}
#endif
	return rtvHeap.hCPU(slice);
}
/*
void RenderTexture::SetUAV(ID3D12GraphicsCommandList* commandList, bool value)
{
	CD3DX12_RESOURCE_BARRIER result;
	ZeroMemory(&result, sizeof(result));
	D3D12_RESOURCE_BARRIER &barrier = result;
	result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = mColorResource.Get();

	if (value)
		commandList->ResourceBarrier(1, &result);
	else
		commandList->ResourceBarrier(1, &result);
}
*/

RenderTexture::RenderTexture(
	ID3D12Device* device,
	UINT width,
	UINT height,
	RenderTextureFormat rtFormat,
	RenderTextureDimension type,
	UINT depthCount,
	UINT mipCount,
	RenderTextureState initState
) : MObject(),
usage(rtFormat.usage),
mType(type),
mWidth(width),
mHeight(height),
mViewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f }),
mScissorRect({ 0, 0, (int)width, (int)height })
{
	UINT arraySize;
	switch (type)
	{
	case RenderTextureDimension::RenderTextureDimension_Cubemap:
		arraySize = 6;
		break;
	case RenderTextureDimension::RenderTextureDimension_Tex2D:
		arraySize = 1;
		break;
	default:
		arraySize = max<uint>(1, depthCount);
		break;
	}
	if (rtFormat.usage == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
	{
		mFormat = rtFormat.colorFormat;


		mipCount = max<uint>(1, mipCount);
		this->mipCount = mipCount;
		depthSlice = arraySize;
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = (type == RenderTextureDimension::RenderTextureDimension_Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = mWidth;
		texDesc.Height = mHeight;
		texDesc.DepthOrArraySize = arraySize;
		texDesc.MipLevels = mipCount;
		texDesc.Format = mFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = mFormat;
		memset(clearValue.Color, 0, sizeof(float) * 4);
		this->initState = (type == RenderTextureDimension::RenderTextureDimension_Tex3D || initState == RenderTextureState::Unordered_Access)
			? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_RENDER_TARGET;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			this->initState,
			&clearValue,
			IID_PPV_ARGS(&mColorResource)));
		if (type != RenderTextureDimension_Tex3D)
			rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, arraySize, false);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		switch (type)
		{
		case RenderTextureDimension::RenderTextureDimension_Tex2D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(0));
			break;

		case RenderTextureDimension::RenderTextureDimension_Tex3D:
			break;
		default:

			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 1;
			for (int i = 0; i < arraySize; ++i)
			{
				// Render target to ith element.
				rtvDesc.Texture2DArray.FirstArraySlice = i;
				// Create RTV to ith cubemap face.
				device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(i));
			}
			break;
		}
	}
	else
	{
		DXGI_FORMAT mDepthFormat;
		switch (rtFormat.depthFormat)
		{
		case RenderTextureDepthSettings_Depth32:
			mFormat = DXGI_FORMAT_R32_FLOAT;
			mDepthFormat = DXGI_FORMAT_D32_FLOAT;
			break;
		case RenderTextureDepthSettings_Depth16:
			mFormat = DXGI_FORMAT_R16_UNORM;
			mDepthFormat = DXGI_FORMAT_D16_UNORM;
			break;
		case RenderTextureDepthSettings_DepthStencil:
			mFormat = DXGI_FORMAT_R24G8_TYPELESS;
			mDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		default:
			mFormat = DXGI_FORMAT_UNKNOWN;
			mDepthFormat = DXGI_FORMAT_UNKNOWN;
			break;
		}

		if (mFormat != DXGI_FORMAT_UNKNOWN)
		{
			rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, arraySize, false);
			D3D12_RESOURCE_DESC depthStencilDesc;
			depthStencilDesc.Dimension = (type == RenderTextureDimension::RenderTextureDimension_Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthStencilDesc.Alignment = 0;
			depthStencilDesc.Width = width;
			depthStencilDesc.Height = height;
			depthStencilDesc.DepthOrArraySize = arraySize;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.Format = mFormat;
			mFormat = mDepthFormat;
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			this->initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&depthStencilDesc,
				this->initState,
				nullptr,
				IID_PPV_ARGS(mColorResource.GetAddressOf())));
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			switch (type)
			{
			case RenderTextureDimension::RenderTextureDimension_Tex2D:
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Format = mDepthFormat;
				dsvDesc.Texture2D.MipSlice = 0;
				device->CreateDepthStencilView(mColorResource.Get(), &dsvDesc, rtvHeap.hCPU(0));
				break;
			default:
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.MipSlice = 0;
				for (int i = 0; i < arraySize; ++i)
				{
					dsvDesc.Texture2DArray.FirstArraySlice = i;
					device->CreateDepthStencilView(mColorResource.Get(), &dsvDesc, rtvHeap.hCPU(i));
				}
				break;

			}

		}

	}
}

void RenderTexture::BindRTVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT slice)
{
	if (usage == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		switch (mType)
		{
		case RenderTextureDimension::RenderTextureDimension_Tex2D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, targetHeap->hCPU(index));
			break;

		case RenderTextureDimension::RenderTextureDimension_Tex3D:
			break;
		default:

			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.FirstArraySlice = slice;
			device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, targetHeap->hCPU(index));
			break;
		}
	}
	else
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		switch (mType)
		{
		case RenderTextureDimension::RenderTextureDimension_Tex2D:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = mFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			device->CreateDepthStencilView(mColorResource.Get(), &dsvDesc, targetHeap->hCPU(index));
			break;
		default:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

			dsvDesc.Texture2DArray.ArraySize = 1;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = slice;
			device->CreateDepthStencilView(mColorResource.Get(), &dsvDesc, targetHeap->hCPU(index));

			break;

		}
	}
}

void RenderTexture::BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetColorViewDesc(srvDesc);
	device->CreateShaderResourceView(mColorResource.Get(), &srvDesc, targetHeap->hCPU(index));
}

void RenderTexture::BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	GetColorUAVDesc(uavDesc, targetMipLevel);
	device->CreateUnorderedAccessView(mColorResource.Get(), nullptr, &uavDesc, targetHeap->hCPU(index));
}