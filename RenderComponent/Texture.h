#pragma once
#include "../Common/d3dUtil.h"
#include <string>
#include <vector>
#include "../Common/MObject.h"
class FrameResource;
class DescriptorHeap;
enum class TextureType : int
{
	Tex2D = 0,
	Tex3D = 1,
	Cubemap = 2,
	Num = 3
};

struct TextureData
{
	UINT width;
	UINT height;
	UINT depth;
	TextureType textureType;
	UINT mipCount;
	enum LoadFormat
	{
		LoadFormat_RGBA8 = 0,
		LoadFormat_RGBA16 = 1,
		LoadFormat_RGBAFloat16 = 2,
		LoadFormat_RGBAFloat32 = 3,
		LoadFormat_Num = 4
	};
	//TODO
	//Should Have Compress Type here
	LoadFormat format;
};

class Texture : public MObject
{
public:

private:
	std::wstring Filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	TextureType mType;
	DXGI_FORMAT mFormat;
	UINT16 mipLevels;
	void GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
public:

	std::string Name;
	inline bool isAvaliable() const { return Resource != nullptr; }
	inline ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}
	Texture(
		ID3D12Device* device,
		FrameResource* res,
		const std::string& name,
		const std::wstring& filePath,
		TextureType type = TextureType::Tex2D
	);
	void BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
};

