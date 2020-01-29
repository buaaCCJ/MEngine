#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class FrameResource;
class UploadBuffer : public MObject
{
public:
	void Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride);
	UploadBuffer() : MObject() {}
	virtual ~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);
	}
	inline void CopyData(UINT elementIndex, const void* data)
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = elementIndex * mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, mStride);
	}
	inline void CopyDataInside(UINT from, UINT to)
	{
		char* dataPos = (char*)mMappedData;
		size_t fromOffset = from * mElementByteSize;
		size_t toOffset = to * mElementByteSize;
		memcpy(dataPos + toOffset, dataPos + fromOffset, mStride);
	}
	inline void CopyDatas(UINT startElementIndex, UINT elementCount, const void* data)
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = startElementIndex * mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, (elementCount - 1) * mElementByteSize + mStride);
	}
	inline void CopyFrom(UploadBuffer* otherBuffer, UINT selfStartIndex, UINT otherBufferStartIndex, UINT elementCount)
	{
		char* otherPtr = (char*)otherBuffer->mMappedData;
		char* currPtr = (char*)mMappedData;
		otherPtr += otherBuffer->mElementByteSize * otherBufferStartIndex;
		currPtr += mElementByteSize * selfStartIndex;
		memcpy(currPtr, otherPtr, elementCount * mElementByteSize);
	}
	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT elementCount)
	{
		return mUploadBuffer->GetGPUVirtualAddress() + elementCount * mElementByteSize;
	}
	constexpr void* GetMappedDataPtr(UINT element)
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = element * mElementByteSize;
		dataPos += offset;
		return dataPos;
	}
	constexpr size_t GetStride() const { return mStride; }
	constexpr size_t GetAlignedStride() const { return mElementByteSize; }
	inline ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}
	inline UINT GetElementCount() const
	{
		return mElementCount;
	}
	void ReleaseAfterFlush(FrameResource* res);
private:
	struct UploadCommand
	{
		UINT startIndex;
		UINT count;
	};
	void* mMappedData = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer = nullptr;
	size_t mStride = 0;
	UINT mElementCount = 0;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};