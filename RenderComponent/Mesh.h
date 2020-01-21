#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class FrameResource;
class Mesh : public MObject
{
	Microsoft::WRL::ComPtr<ID3D12Resource> dataBuffer = nullptr;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT meshLayoutIndex;
	UINT mVertexCount;
	std::array<int, 8> offsets;
	DXGI_FORMAT indexFormat;
	UINT indexCount;
	void* indexArrayPtr;
public:
	UINT GetIndexCount() const { return indexCount; }
	UINT GetIndexFormat() const { return indexFormat; }
	DirectX::XMFLOAT3 boundingCenter = { 0,0,0 };
	DirectX::XMFLOAT3 boundingExtent = {0.5f,0.5f,0.5f};
	inline UINT GetLayoutIndex() const { return meshLayoutIndex; }
	inline UINT GetVertexCount() const { return mVertexCount; }
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView();
	Mesh(
		int vertexCount,
		DirectX::XMFLOAT3* positions,
		DirectX::XMFLOAT3* normals,
		DirectX::XMFLOAT4* tangents,
		DirectX::XMFLOAT4* colors,
		DirectX::XMFLOAT2* uv,
		DirectX::XMFLOAT2* uv1,
		DirectX::XMFLOAT2* uv2,
		DirectX::XMFLOAT2* uv3,
		ID3D12Device* device,
		DXGI_FORMAT indexFormat,
		UINT indexCount,
		void* indexArrayPtr
	);
	static ObjectPtr<Mesh> LoadMeshFromFile(const std::wstring& str, ID3D12Device* device);
};