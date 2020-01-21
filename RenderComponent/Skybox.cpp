#include "Skybox.h"
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <mutex>
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "MeshRenderer.h"
#include "../Common/Camera.h"
#include "../Singleton/PSOContainer.h"
#include "DescriptorHeap.h"
#include "../LogicComponent/World.h"

std::unique_ptr<Mesh> Skybox::fullScreenMesh = nullptr;
void Skybox::Draw(
	int targetPass,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	FrameResource* currentResource,
	PSOContainer* container
)
{
	UINT value = cameraBuffer->element;
	PSODescriptor desc;
	desc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	shader->BindRootSignature(commandList, World::GetInstance()->GetGlobalDescHeap());
	shader->SetResource(commandList, ShaderID::GetMainTex(), World::GetInstance()->GetGlobalDescHeap(), descHeapIndex);
	shader->SetResource(commandList, SkyboxCBufferID, cameraBuffer->buffer, cameraBuffer->element);
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}

Skybox::~Skybox()
{
	fullScreenMesh = nullptr;
	skyboxTex = nullptr;
	World::GetInstance()->ReturnDescHeapIndexToPool(descHeapIndex);
}

Skybox::Skybox(
	ObjectPtr<Texture>& tex,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList
) : skyboxTex(tex)
{
	World* world = World::GetInstance();
	SkyboxCBufferID = ShaderID::PropertyToID("SkyboxBuffer");
	descHeapIndex = world->GetDescHeapIndexFromPool();
	tex->BindColorBufferToSRVHeap(world->GetGlobalDescHeap(), descHeapIndex, device);
	ObjectPtr<UploadBuffer> noProperty = nullptr;
	shader = ShaderCompiler::GetShader("Skybox");
	if (fullScreenMesh == nullptr) {
		std::array<DirectX::XMFLOAT3, 3> vertex;
		vertex[0] = { -3, -1, 1 };
		vertex[1] = { 1, 3, 1 };
		vertex[2] = { 1, -1, 1 };
		std::array<INT16, 3> indices{ 0, 1, 2 };
		fullScreenMesh = std::unique_ptr<Mesh>(new Mesh(
			3,
			vertex.data(),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			device,
			DXGI_FORMAT_R16_UINT,
			3,
			indices.data()
			));
	}
}