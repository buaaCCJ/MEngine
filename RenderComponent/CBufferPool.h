#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class UploadBuffer;
struct ConstBufferElement
{
	UploadBuffer* buffer;
	UINT element;
};
class CBufferPool
{
private:
	std::vector<UploadBuffer*> arr;
	std::vector<ConstBufferElement> poolValue;
	UINT capacity;
	UINT stride;
	void Add(ID3D12Device* device);
public:
	CBufferPool(UINT stride, UINT initCapacity);
	~CBufferPool();
	ConstBufferElement Get(ID3D12Device* device);
	void Return(ConstBufferElement& target);
};