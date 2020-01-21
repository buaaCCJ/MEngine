#pragma once
#include "../Common/d3dUtil.h"
#include "CBufferPool.h"
#include "../Common/MObject.h"
class FrameResource;
class MaterialManager;
class Material : public MObject
{
	friend class MaterialManager;
private:
	MaterialManager* manager;
	UINT cbufferEle;
public:
	Material(MaterialManager* manager, ID3D12Device* device, FrameResource* resource);
	virtual ~Material();
	constexpr UINT GetMaterialID() { return cbufferEle; }
	template <typename T>
	constexpr void CopyData(T& data);
};


#include "UploadBuffer.h"

template <typename T>
constexpr void Material::CopyData(T& data)
{

#ifndef NDEBUG
	if (sizeof(T) != manager->propertySize) throw "Struct size not the same!";
#endif
	cbuffer.buffer->CopyData(cbuffer.element, &data);
}