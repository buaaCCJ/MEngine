#pragma once
#include "../Common/MObject.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/MetaLib.h"
#include "../RenderComponent/StructuredBuffer.h"
struct ResourceDescriptor
{
	enum ResourceType
	{
		ResourceType_RenderTexture,
		ResourceType_StructuredBuffer
	};
	ResourceType type = ResourceType_RenderTexture;
	union
	{
		StructuredBufferElement sbufDesc;
		RenderTextureDescriptor rtDesc;
	};
	bool operator!=(const ResourceDescriptor& res) const;
	bool operator==(const ResourceDescriptor& res) const;
};
namespace std
{
	template <>
	class hash<ResourceDescriptor>
	{
	public:
		size_t operator()(const ResourceDescriptor& o) const
		{
			hash<UINT> hashFunc;
			if (o.type == ResourceDescriptor::ResourceType_RenderTexture)
			{
				return hashFunc(
					(UINT)o.type ^
					o.rtDesc.width ^
					o.rtDesc.height ^
					o.rtDesc.depthSlice ^
					(UINT)o.rtDesc.type ^
					(o.rtDesc.rtFormat.usage == RenderTextureUsage::RenderTextureUsage_ColorBuffer
						? (UINT)o.rtDesc.rtFormat.colorFormat
						: (UINT)o.rtDesc.rtFormat.depthFormat));


			}
			else
			{
				return hashFunc(
					(UINT)o.type ^
					(UINT)o.sbufDesc.elementCount ^
					(UINT)o.sbufDesc.stride);
			}
		}
	};

}
class TempRTAllocator
{
public:
	struct UsingTempRT
	{
		MObject* rt;
		ResourceDescriptor desc;
	};
private:
	struct TempRTData
	{
		MObject* rt;
		UINT containedFrame;
	};
	Dictionary<ResourceDescriptor, std::vector<TempRTData>*> waitingRT;
	std::unordered_map<UINT, UsingTempRT> usingRT;
public:
	TempRTAllocator();
	~TempRTAllocator();
	bool Contains(UINT id);
	MObject* GetUsingRenderTexture(UINT id);
	MObject* GetTempResource(ID3D12Device* device, UINT id, ResourceDescriptor& descriptors);
	void ReleaseRenderTexutre(UINT id);
	void CumulateReleaseAfterFrame();
	//TODO
};

