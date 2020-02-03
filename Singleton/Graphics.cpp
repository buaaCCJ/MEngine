#include "Graphics.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Shader.h"
#include "MeshLayout.h"
#include "../RenderComponent/Mesh.h"
#include "ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/RenderTexture.h"
std::unique_ptr<Mesh> fullScreenMesh(nullptr);
void Graphics::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{

	std::array<DirectX::XMFLOAT3, 3> vertex;
	std::array<DirectX::XMFLOAT2, 3> uv;
	vertex[0] = { -3, -1, 1 };
	vertex[1] = { 1, -1, 1 };
	vertex[2] = { 1, 3, 1 };
	uv[0] = { -1, 1 };
	uv[1] = { 1, 1 };
	uv[2] = { 1, -1 };
	std::array<INT16, 3> indices = { 0, 1, 2 };
	fullScreenMesh = std::unique_ptr<Mesh>(new Mesh(
		3,
		vertex.data(),
		nullptr,
		nullptr,
		nullptr,
		uv.data(),
		nullptr,
		nullptr,
		nullptr,
		device,
		DXGI_FORMAT_R16_UINT,
		3,
		indices.data()
		));

}


void Graphics::CopyTexture(
	ID3D12GraphicsCommandList* commandList,
	RenderTexture* source, UINT sourceSlice, UINT sourceMipLevel,
	RenderTexture* dest,  UINT destSlice, UINT destMipLevel)
{
	if (source->GetType() == RenderTextureDimension_Tex2D) sourceSlice = 0;
	if (dest->GetType() == RenderTextureDimension_Tex2D) destSlice = 0;
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	sourceLocation.SubresourceIndex = sourceSlice * source->GetMipCount() + sourceMipLevel;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = destSlice * dest->GetMipCount() + destMipLevel;
	sourceLocation.pResource = source->GetColorResource();
	destLocation.pResource = dest->GetColorResource();
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
}

void Graphics::CopyBufferToTexture(
	ID3D12GraphicsCommandList* commandList,
	UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
	ID3D12Resource* textureResource, UINT targetMip,
	UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize)
{
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.pResource = sourceBuffer->Resource();
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceLocation.PlacedFootprint.Offset = sourceBufferOffset;
	sourceLocation.PlacedFootprint.Footprint =
	{
		targetFormat, //DXGI_FORMAT Format;
		width, //UINT Width;
		height, //UINT Height;
		depth, //UINT Depth;
		d3dUtil::CalcConstantBufferByteSize(width * pixelSize)//UINT RowPitch;
	};
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = targetMip;
	destLocation.pResource = textureResource;
	ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST, textureResource);
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
	ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ, textureResource);
}

void Graphics::Blit(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
	UINT renderTargetCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
	PSOContainer* container, uint containerIndex,
	UINT width, UINT height,
	Shader* shader, UINT pass)
{
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (int)width, (int)height };
	PSODescriptor psoDesc;
	psoDesc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	psoDesc.shaderPass = pass;
	psoDesc.shaderPtr = shader;
	commandList->OMSetRenderTargets(renderTargetCount, renderTarget, false, depthTarget);
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->SetPipelineState(container->GetState(psoDesc, device, containerIndex));
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}
