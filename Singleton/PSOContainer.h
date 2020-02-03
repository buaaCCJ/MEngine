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
	template <>
	struct hash<std::pair<uint, PSODescriptor>>
	{
		size_t operator()(const std::pair<uint, PSODescriptor>& key) const
		{
			return key.first ^ key.second.hash;
		}
	};
}
struct PSORTSetting
{
	DXGI_FORMAT depthFormat;
	UINT rtCount;
	DXGI_FORMAT rtFormat[8];
};
class PSOContainer
{
private:
	std::unordered_map<std::pair<uint, PSODescriptor>, Microsoft::WRL::ComPtr<ID3D12PipelineState>> allPSOState;
	
	std::vector<PSORTSetting> settings;
public:
	PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat);
	PSOContainer(PSORTSetting* formats, uint formatCount);
	ID3D12PipelineState* GetState(PSODescriptor& desc, ID3D12Device* device, uint index);
};