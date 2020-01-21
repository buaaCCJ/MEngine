#pragma once
#include "../Common/d3dUtil.h"
class UploadBuffer;
class Shader;
class Material;
class FrameResource;
class MaterialManager
{
	friend class Material;
private:
	Shader* mShader;
	UINT propertySize;
	std::unique_ptr<UploadBuffer> pool;
	std::vector<UINT> elementPool;
	UINT GetElement(FrameResource* resource, ID3D12Device* device);
	void ReturnElement(UINT ele);
public:
	MaterialManager(
		Shader* shader,
		UINT propertySize,
		ID3D12Device* device
	);
	virtual ~MaterialManager() {}

};