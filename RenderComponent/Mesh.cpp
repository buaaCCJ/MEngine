#include "Mesh.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/MeshLayout.h"
#include "../Singleton/Graphics.h"
#include "../RenderComponent/RenderCommand.h"
using namespace DirectX;
using Microsoft::WRL::ComPtr;
//CPP
D3D12_VERTEX_BUFFER_VIEW Mesh::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = dataBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}


D3D12_INDEX_BUFFER_VIEW Mesh::IndexBufferView()
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = dataBuffer->GetGPUVirtualAddress() + VertexBufferByteSize;
	ibv.Format = indexFormat;
	ibv.SizeInBytes = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
	return ibv;
}

ComPtr<ID3D12Resource> CreateDefaultBuffer(
	ID3D12Device* device,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));


	return defaultBuffer;
}

void CopyToBuffer(
	UINT64 byteSize,
	ID3D12GraphicsCommandList* cmdList,
	ComPtr<ID3D12Resource>& uploadBuffer,
	ComPtr<ID3D12Resource>& defaultBuffer)
{
	Graphics::ResourceStateTransform(cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, defaultBuffer.Get());
	cmdList->CopyBufferRegion(defaultBuffer.Get(), 0, uploadBuffer.Get(), 0, byteSize);
	Graphics::ResourceStateTransform(cmdList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ, defaultBuffer.Get());

}

class MeshLoadCommand : public RenderCommand
{
public:
	ComPtr<ID3D12Resource> uploadResource;
	ComPtr<ID3D12Resource> defaultResource;
	UINT64 byteSize;
	MeshLoadCommand(
		ComPtr<ID3D12Resource>& uploadResource,
		ComPtr<ID3D12Resource>& defaultResource,
		UINT64 byteSize
	) : byteSize(byteSize), uploadResource(uploadResource), defaultResource(defaultResource)
	{}

	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource)
	{
		FrameResource::ReleaseResourceAfterFlush(uploadResource, resource);
		FrameResource::ReleaseResourceAfterFlush(defaultResource, resource);
		CopyToBuffer(byteSize, commandList, uploadResource, defaultResource);
	}
};

Mesh::Mesh(
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
) : MObject(), mVertexCount(vertexCount),
indexFormat(indexFormat),
indexCount(indexCount),
indexArrayPtr(indexArrayPtr)
{
	//IndexFormat = indexFormat == DXGI_FORMAT_R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	//TODO
	meshLayoutIndex = MeshLayout::GetMeshLayoutIndex(
		normals != nullptr,
		tangents != nullptr,
		colors != nullptr,
		uv != nullptr,
		uv1 != nullptr,
		uv2 != nullptr,
		uv3 != nullptr,
		false
	);
	std::vector<D3D12_INPUT_ELEMENT_DESC>* meshLayouts = MeshLayout::GetMeshLayoutValue(meshLayoutIndex);
	UINT stride = 0;
	auto cumulate = [&](void* ptr, UINT size) -> void
	{
		if (ptr != nullptr) stride += size;
	};
	cumulate(positions, 12);
	cumulate(normals, 12);
	cumulate(tangents, 16);
	cumulate(colors, 16);
	cumulate(uv, 8);
	cumulate(uv1, 8);
	cumulate(uv2, 8);
	cumulate(uv3, 8);
	VertexByteStride = stride;
	VertexBufferByteSize = stride * vertexCount;
	//IndexBufferByteSize = (IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4) * indexCount;
	size_t indexSize = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
	char* dataPtr = reinterpret_cast<char*>(malloc(VertexBufferByteSize + indexSize));
	std::unique_ptr<char> dataPtrGuard(dataPtr);
	auto vertBufferCopy = [&](char* buffer, char* ptr, UINT size, int& offset, std::array<int, 8>& offstArray, int arrayLen) -> void
	{
		if (ptr == nullptr)
		{
			offstArray[arrayLen] = -1;
			return;
		}
		for (int i = 0; i < vertexCount; ++i)
		{
			memcpy(buffer + i * stride + offset, ptr + size * i, size);
		}
		offstArray[arrayLen] = offset;
		offset += size;
	};
	int offset = 0;
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(positions),
		12,
		offset,
		offsets,
		0
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(normals),
		12,
		offset,
		offsets,
		1
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(tangents),
		16,
		offset,
		offsets,
		2
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(colors),
		16,
		offset,
		offsets,
		3
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv),
		8,
		offset,
		offsets,
		4
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv1),
		8,
		offset,
		offsets,
		5
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv2),
		8,
		offset,
		offsets,
		6
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv3),
		8,
		offset,
		offsets,
		7
	);
	char* indexBufferStart = dataPtr + VertexBufferByteSize;
	memcpy(indexBufferStart, indexArrayPtr, indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4));
	ComPtr<ID3D12Resource> uploadBuffer;
	dataBuffer = CreateDefaultBuffer(device, indexSize + VertexBufferByteSize, uploadBuffer);
	void* mappedPtr = nullptr;
	ThrowIfFailed(uploadBuffer->Map(0, nullptr, &mappedPtr));
	memcpy(mappedPtr, dataPtr, indexSize + VertexBufferByteSize);
	uploadBuffer->Unmap(0, nullptr);
	MeshLoadCommand* meshLoadCommand = new MeshLoadCommand(
		uploadBuffer,
		dataBuffer,
		indexSize + VertexBufferByteSize
	);
	RenderCommand::AddCommand(meshLoadCommand);
}

struct IndexSettings
{
	enum IndexFormat
	{
		IndexFormat_16Bit = 0,
		IndexFormat_32Bit = 1
	};
	IndexFormat indexFormat;
	UINT indexCount;
};
struct MeshHeader
{
	enum MeshDataType
	{
		MeshDataType_Vertex = 0,
		MeshDataType_Index = 1,
		MeshDataType_Normal = 2,
		MeshDataType_Tangent = 3,
		MeshDataType_UV = 4,
		MeshDataType_UV2 = 5,
		MeshDataType_UV3 = 6,
		MeshDataType_UV4 = 7,
		MeshDataType_Color = 8,
		MeshDataType_BoneIndex = 9,
		MeshDataType_BoneWeight = 10,
		MeshDataType_BoundingBox = 11,
		MeshDataType_Num = 12
	};
	MeshDataType type;
	union
	{
		IndexSettings indexSettings;
		UINT vertexCount;
		UINT normalCount;
		UINT tangentCount;
		UINT uvCount;
		UINT colorCount;
		UINT boneCount;
	};
};

struct MeshData
{
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
	std::vector<XMFLOAT3> vertex;
	std::vector<XMFLOAT3> normal;
	std::vector<XMFLOAT4> tangent;
	std::vector<XMFLOAT2> uv;
	std::vector<XMFLOAT2> uv2;
	std::vector<XMFLOAT2> uv3;
	std::vector<XMFLOAT2> uv4;
	std::vector<XMFLOAT4> color;
	std::vector<XMUINT4> boneIndex;
	std::vector<XMFLOAT4> boneWeight;
	std::vector<char> indexData;
	XMFLOAT3 boundingCenter;
	XMFLOAT3 boundingExtent;
};

bool DecodeMesh(
	const std::wstring& filePath,
	MeshData& meshData)
{
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs)
	{
		return false;//File Read Error!
	}
	UINT chunkCount = 0;
	ifs.read((char*)&chunkCount, sizeof(UINT));
	if (chunkCount >= MeshHeader::MeshDataType_Num) return false;	//Too many types
	for (UINT i = 0; i < chunkCount; ++i)
	{
		MeshHeader header;
		ifs.read((char*)&header, sizeof(MeshHeader));
		if (header.type >= MeshHeader::MeshDataType_Num) return false;	//Illegal Data Type
		size_t indexSize;
		switch (header.type)
		{
		case MeshHeader::MeshDataType_Vertex:
			meshData.vertex.resize(header.vertexCount);
			ifs.read((char*)meshData.vertex.data(), sizeof(XMFLOAT3) * header.vertexCount);
			break;
		case MeshHeader::MeshDataType_Normal:
			meshData.normal.resize(header.normalCount);
			ifs.read((char*)meshData.normal.data(), sizeof(XMFLOAT3) * header.normalCount);
			break;
		case MeshHeader::MeshDataType_Tangent:
			meshData.tangent.resize(header.tangentCount);
			ifs.read((char*)meshData.tangent.data(), sizeof(XMFLOAT4) * header.tangentCount);
			break;
		case MeshHeader::MeshDataType_UV:
			meshData.uv.resize(header.uvCount);
			ifs.read((char*)meshData.uv.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV2:
			meshData.uv2.resize(header.uvCount);
			ifs.read((char*)meshData.uv2.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV3:
			meshData.uv3.resize(header.uvCount);
			ifs.read((char*)meshData.uv3.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV4:
			meshData.uv4.resize(header.uvCount);
			ifs.read((char*)meshData.uv4.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_BoneIndex:
			meshData.boneIndex.resize(header.boneCount);
			ifs.read((char*)meshData.boneIndex.data(), sizeof(XMUINT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_BoneWeight:
			meshData.boneWeight.resize(header.boneCount);
			ifs.read((char*)meshData.boneWeight.data(), sizeof(XMFLOAT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_Index:
			indexSize = header.indexSettings.indexFormat == IndexSettings::IndexFormat_16Bit ? 2 : 4;
			meshData.indexFormat = indexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			meshData.indexData.resize(indexSize * header.indexSettings.indexCount);
			ifs.read(meshData.indexData.data(), meshData.indexData.size());
			break;
		case MeshHeader::MeshDataType_BoundingBox:
			ifs.read((char*)&meshData.boundingCenter, sizeof(XMFLOAT3) * 2);
			break;
		}
	}
	return true;
}

ObjectPtr<Mesh> Mesh::LoadMeshFromFile(const std::wstring& str, ID3D12Device* device)
{
	ObjectPtr<Mesh> result;
	MeshData meshData;
	if (!DecodeMesh(str, meshData)) return result;
	result = new Mesh(
		meshData.vertex.size(),
		meshData.vertex.data(),
		meshData.normal.data(),
		meshData.tangent.data(),
		meshData.color.data(),
		meshData.uv.data(),
		meshData.uv2.data(),
		meshData.uv3.data(),
		meshData.uv4.data(),
		device,
		meshData.indexFormat,
		meshData.indexData.size() / ((meshData.indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4),
		meshData.indexData.data()
	);
	result->boundingCenter = meshData.boundingCenter;
	result->boundingExtent = meshData.boundingExtent;
	return result;
}