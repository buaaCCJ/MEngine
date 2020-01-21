#include "TempRTAllocator.h"

TempRTAllocator::TempRTAllocator()
{
	waitingRT.Reserve(30);
}

bool ResourceDescriptor::operator==(const ResourceDescriptor& res) const
{
	if (type != res.type) return false;
	if (type == ResourceDescriptor::ResourceType_RenderTexture)
		return res.rtDesc == rtDesc;
	else
	{
		return res.sbufDesc.elementCount == sbufDesc.elementCount &&
			res.sbufDesc.stride == sbufDesc.stride;
	}
}

bool ResourceDescriptor::operator!=(const ResourceDescriptor& res) const
{
	return !operator==(res);
}

TempRTAllocator::~TempRTAllocator()
{
	for (int i = 0; i < waitingRT.values.size(); ++i)
	{
		auto& v = waitingRT.values[i];
		for (int j = 0; j < v.value->size(); ++j)
			delete (*v.value)[j].rt;
		delete v.value;
	}
}
MObject* TempRTAllocator::GetTempResource(ID3D12Device* device, UINT id, ResourceDescriptor& descriptor)
{

	std::vector<TempRTData>* datas = nullptr;
	std::vector<TempRTData>** datasPtr = waitingRT[descriptor];
	if (datasPtr == nullptr)
	{
		datas = new std::vector<TempRTData>();
		datas->reserve(10);
		waitingRT.Add(descriptor, datas);
	}
	else
		datas = *datasPtr;
	if (datas->size() > 0)
	{
		auto&& dataIte = datas->end() - 1;
		TempRTData& data = *dataIte;
		UsingTempRT usingRTData;
		usingRTData.rt = data.rt;
		usingRTData.desc = descriptor;
		usingRT.insert_or_assign(id, usingRTData);
		MObject* result = data.rt;
		datas->erase(dataIte);
		return result;
	}
	else
	{
		UsingTempRT usingRTData;
		if (descriptor.type == ResourceDescriptor::ResourceType_RenderTexture)
		{
			RenderTextureDescriptor& rtDesc = descriptor.rtDesc;
			usingRTData.rt = new RenderTexture(device, rtDesc.width, rtDesc.height, rtDesc.rtFormat,rtDesc.type, rtDesc.depthSlice, 1);;
		}
		else
		{
			StructuredBufferElement& sbufDesc = descriptor.sbufDesc;
			usingRTData.rt = new StructuredBuffer(device, &sbufDesc, 1);
		}
		usingRTData.desc = descriptor;
		usingRT.insert_or_assign(id, usingRTData);
		return usingRTData.rt;
	}
}
bool TempRTAllocator::Contains(UINT id)
{
	return usingRT.find(id) != usingRT.end();
}
MObject* TempRTAllocator::GetUsingRenderTexture(UINT id)
{
	auto&& ite = usingRT.find(id);
	if (ite != usingRT.end())
	{

		return ite->second.rt;
	}
	return nullptr;
}

void TempRTAllocator::ReleaseRenderTexutre(UINT id)
{
	auto&& ite = usingRT.find(id);
	if (ite != usingRT.end())
	{
		std::vector<TempRTData>** datasPtr = waitingRT[ite->second.desc];
		
		if (datasPtr != nullptr)
		{
			std::vector<TempRTData>* data = *datasPtr;
			TempRTData rtData;
			rtData.rt = ite->second.rt;
			rtData.containedFrame = 0;
			data->emplace_back(rtData);
			int size = data->size();
			size = data->size();
		}
		usingRT.erase(ite);
	}
}

void TempRTAllocator::CumulateReleaseAfterFrame()
{
	for (UINT i = 0; i < waitingRT.values.size(); ++i)
	{
		std::vector<TempRTData>* data = waitingRT.values[i].value;
		for (UINT j = 0; j < data->size(); ++j)
		{
			TempRTData& d = (*data)[j];
			d.containedFrame++;
			if (d.containedFrame >= 3)
			{
				MObject* ptr = d.rt;
				(*data)[j] = (*data)[data->size() - 1];
				data->erase(data->end() - 1);
				j--;
				delete ptr;
			}
		}
	}
}