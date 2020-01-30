#include "Texture.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/DescriptorHeap.h"
#include <fstream>
#include "ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "RenderCommand.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/Graphics.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;



void ReadData(const std::wstring& str, TextureData& headerResult, std::vector<char>& dataResult)
{
	std::ifstream ifs;
	ifs.open(str, std::ios::binary);
	if (!ifs)
	{
		throw "File Not Exists!";
	}
	ifs.read((char*)&headerResult, sizeof(TextureData));
	if (headerResult.mipCount < 1) headerResult.mipCount = 1;
	UINT formt = (UINT)headerResult.format;
	if (formt >= (UINT)(TextureData::LoadFormat_Num) ||
		(UINT)headerResult.textureType >= (UINT)TextureType::Num)
	{
		throw "Invalide Format";
	}
	UINT stride = 0;
	switch (headerResult.format)
	{
	case TextureData::LoadFormat_RGBA8:
		stride = 4;
		break;
	case TextureData::LoadFormat_RGBA16:
		stride = 8;
		break;
	case TextureData::LoadFormat_RGBAFloat16:
		stride = 8;
		break;
	case TextureData::LoadFormat_RGBAFloat32:
		stride = 16;
		break;
	case TextureData::LoadFormat_RG16:
		stride = 4;
		break;
	case TextureData::LoadFormat_RGFLOAT16:
		stride = 4;
		break;
	}
	size_t size = 0;
	UINT depth = headerResult.depth;

	for (UINT j = 0; j < depth; ++j)
	{
		UINT width = headerResult.width;
		UINT height = headerResult.height;
		for (UINT i = 0; i < headerResult.mipCount; ++i)
		{
			UINT currentChunkSize = stride * width * height;
			size += max(currentChunkSize, 512);
			width /= 2;
			height /= 2;
			width = max(1, width);
			height = max(1, height);
		}
	}
	dataResult.resize(size);
	ifs.read(dataResult.data(), size);
}
struct DispatchCBuffer
{
	XMUINT2 _StartPos;
	XMUINT2 _Count;
};

class TextureLoadCommand : public RenderCommand
{
private:
	UploadBuffer ubuffer;
	ID3D12Resource* res;
	TextureData::LoadFormat loadFormat;
	UINT width;
	UINT height;
	UINT mip;
	UINT arraySize;
	TextureType type;
public:
	TextureLoadCommand(ID3D12Device* device,
		UINT element,
		void* dataPtr,
		ID3D12Resource* res,
		TextureData::LoadFormat loadFormat,
		UINT width,
		UINT height,
		UINT mip,
		UINT arraySize,
		TextureType type) :
		res(res), loadFormat(loadFormat), width(width), height(height), mip(mip), arraySize(arraySize), type(type),
		ubuffer(device, element + 2048, false, 1)
	{
		ubuffer.CopyDatas(0, element, dataPtr);
	}

	TextureLoadCommand(
		const UploadBuffer& buffer,
		ID3D12Resource* res,
		TextureData::LoadFormat loadFormat,
		UINT width,
		UINT height,
		UINT mip,
		UINT arraySize,
		TextureType type
	) :res(res), loadFormat(loadFormat), width(width), height(height), mip(mip), arraySize(arraySize), type(type), ubuffer(buffer)
	{

	}
	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource) override
	{
		ubuffer.ReleaseAfterFlush(resource);
		UINT offset = 0;

		DXGI_FORMAT texFormat;
		size_t currentOffset = 0;
		switch (loadFormat)
		{
		case TextureData::LoadFormat_RGBA8:
			texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			currentOffset = 4;
			break;
		case TextureData::LoadFormat_RGBA16:
			texFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			currentOffset = 8;
			break;
		case TextureData::LoadFormat_RGBAFloat32:
			texFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			currentOffset = 16;
			break;
		case TextureData::LoadFormat_RGBAFloat16:
			texFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			currentOffset = 8;
			break;
		case TextureData::LoadFormat_RG16:
			texFormat = DXGI_FORMAT_R16G16_UNORM;
			currentOffset = 4;
			break;
		case TextureData::LoadFormat_RGFLOAT16:
			texFormat = DXGI_FORMAT_R16G16_FLOAT;
			currentOffset = 4;
			break;
		}
		if (type == TextureType::Tex3D)
		{
			UINT curWidth = width;
			UINT curHeight = height;
			for (UINT i = 0; i < mip; ++i)
			{
				Graphics::CopyBufferToTexture(
					commandList,
					&ubuffer,
					offset,
					res,
					i,
					curWidth, curHeight,
					arraySize,
					texFormat, currentOffset
				);
				UINT chunkOffset = currentOffset * curWidth * curHeight;
				offset += max(chunkOffset, 512);
				curWidth /= 2;
				curHeight /= 2;
			}
		}
		else
		{
			for (UINT j = 0; j < arraySize; ++j)
			{
				UINT curWidth = width;
				UINT curHeight = height;
				for (UINT i = 0; i < mip; ++i)
				{
					Graphics::CopyBufferToTexture(
						commandList,
						&ubuffer,
						offset,
						res,
						(j * mip) + i,
						curWidth, curHeight,
						1,
						texFormat, currentOffset
					);
					UINT chunkOffset = currentOffset * curWidth * curHeight;
					offset += max(chunkOffset, 512);
					curWidth /= 2;
					curHeight /= 2;
				}

			}
		}
		size_t ofst = offset;
		size_t size = ubuffer.GetElementCount();
	}
};

Texture::Texture(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	const UploadBuffer& buffer,
	UINT width,
	UINT height,
	UINT depth,
	TextureType textureType,
	UINT mipCount,
	TextureData::LoadFormat format,
	TextureType type) : 
	mType(type)
{
	if (type == TextureType::Cubemap)
		depth = 6;
	switch (format)
	{
	case TextureData::LoadFormat_RGBA8:
		mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureData::LoadFormat_RGBA16:
		mFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case TextureData::LoadFormat_RGBAFloat16:
		mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case TextureData::LoadFormat_RGBAFloat32:
		mFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case TextureData::LoadFormat_RG16:
		mFormat = DXGI_FORMAT_R16G16_UNORM;
		break;
	case TextureData::LoadFormat_RGFLOAT16:
		mFormat = DXGI_FORMAT_R16G16_FLOAT;
		break;
	}
	mipLevels = mipCount;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = depth;
	texDesc.MipLevels = mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Resource)));
	TextureLoadCommand cmd(
		buffer,
		Resource.Get(),
		format,
		width,
		height,
		mipCount,
		depth,
		mType);
	cmd(device, commandList, resource);
}

Texture::Texture(
	ID3D12Device* device,
	const std::string& name,
	const std::wstring& filePath,
	TextureType type
) : MObject(), mType(type),
Name(name),
Filename(filePath)
{

	TextureData data;//TODO : Read From Texture
	ZeroMemory(&data, sizeof(TextureData));

	std::vector<char> dataResults;
	ReadData(filePath, data, dataResults);
	if (data.textureType != type)
		throw "Texture Type Not Match Exception";
	
	if (type == TextureType::Cubemap && data.depth != 6)
		throw "Cubemap's tex size must be 6";
	switch (data.format)
	{
	case TextureData::LoadFormat_RGBA8:
		mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureData::LoadFormat_RGBA16:
		mFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case TextureData::LoadFormat_RGBAFloat16:
		mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case TextureData::LoadFormat_RGBAFloat32:
		mFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case TextureData::LoadFormat_RG16:
		mFormat = DXGI_FORMAT_R16G16_UNORM;
		break;
	case TextureData::LoadFormat_RGFLOAT16:
		mFormat = DXGI_FORMAT_R16G16_FLOAT;
		break;
	}
	mipLevels = data.mipCount;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = data.width;
	texDesc.Height = data.height;
	texDesc.DepthOrArraySize = data.depth;
	texDesc.MipLevels = data.mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Resource)));

	TextureLoadCommand* cmd = new TextureLoadCommand(
		device,
		dataResults.size(),
		dataResults.data(),
		Resource.Get(),
		data.format,
		data.width,
		data.height,
		data.mipCount,
		data.depth,
		data.textureType);
	RenderCommand::AddCommand(cmd);

}
void Texture::GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (mType) {
	case TextureType::Tex2D:
		srvDesc.Format = mFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case TextureType::Tex3D:
		srvDesc.Format = mFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	case TextureType::Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = mFormat;
		break;
	}
}

void Texture::BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetResourceViewDescriptor(srvDesc);
	device->CreateShaderResourceView(Resource.Get(), &srvDesc, targetHeap->hCPU(index));
}