#pragma once
#include "../RenderComponent/Shader.h"
#include <unordered_map>
#include <mutex>
struct PSODescriptor
{
	Shader* shaderPtr;
	UINT shaderPass;
	UINT meshLayoutIndex;
	size_t hash;
	bool operator==(const PSODescriptor& other)const;
	bool operator==(const PSODescriptor&& other)const;
	void GenerateHash()
	{
		size_t value = shaderPass;		
		value += meshLayoutIndex;
		value += reinterpret_cast<size_t>(shaderPtr);
		std::hash<size_t> h;
		hash = h(value);
	}
};
namespace std
{
	template <>
	struct hash<PSODescriptor>
	{
		size_t operator()(const PSODescriptor& key) const
		{
			return key.hash;
		}
	};
}
class PSOContainer
{
private:
	std::unordered_map<PSODescriptor, Microsoft::WRL::ComPtr<ID3D12PipelineState>> allPSOState;
	DXGI_FORMAT depthFormat;
	UINT rtCount;
	DXGI_FORMAT rtFormat[8];
public:
	DXGI_FORMAT* GetColorFormats()
	{
		return rtFormat;
	}
	UINT GetRTCount() const
	{
		return rtCount;
	}
	DXGI_FORMAT GetDepthFormat() const
	{
		return depthFormat;
	}
	PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat);
	ID3D12PipelineState* GetState(PSODescriptor& desc, ID3D12Device* device);
};