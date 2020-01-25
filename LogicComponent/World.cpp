#include "World.h"
#include "../Common/GeometryGenerator.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/GPU Driven/GRP_Renderer.h"
#include "Transform.h"
using namespace DirectX;
World* World::current = nullptr;
void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, FrameResource* res);

World::World(ID3D12GraphicsCommandList* commandList, ID3D12Device* device) :
	usedDescs(MAXIMUM_HEAP_COUNT),
	unusedDescs(MAXIMUM_HEAP_COUNT),
	globalDescriptorHeap(new DescriptorHeap)
{
	globalDescriptorHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAXIMUM_HEAP_COUNT, true);
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}

	GeometryGenerator geoGen;
	mesh = Mesh::LoadMeshFromFile(L"Resource/Wheel.vmesh", device);
	if (!mesh)
	{
		BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), mesh, device, commandList, nullptr);
	}
	trans = new Transform(nullptr);
	//meshRenderer = new MeshRenderer(trans.operator->(), device, mesh, ShaderCompiler::GetShader("OpaqueStandard"));
	grpRenderer = new GRP_Renderer(
		mesh->GetLayoutIndex(),
		256,
		ShaderCompiler::GetShader("OpaqueStandard"),
		device
	);
	grpRenderer->AddRenderElement(
		trans, mesh, device, 0, 0
	);
	/*light = new Light(trans);
	light->SetEnabled(true)*/
}

UINT World::GetDescHeapIndexFromPool()
{
	std::lock_guard lck(mtx);
	auto&& last = unusedDescs.end() - 1;
	UINT value = *last;
	unusedDescs.erase(last);
	usedDescs[value] = true;
	return value;
}

World::~World()
{
	globalDescriptorHeap.Destroy();
}

void World::ReturnDescHeapIndexToPool(UINT target)
{
	std::lock_guard lck(mtx);
	auto ite = usedDescs[target];
	if (ite)
	{
		unusedDescs.push_back(target);
		ite = false;
	}
}

void World::ForceCollectAllHeapIndex()
{
	std::lock_guard lck(mtx);
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
	usedDescs.Clear();
}

void World::Update(FrameResource* resource, ID3D12Device* device)
{
	grpRenderer->UpdateFrame(resource, device);
}


void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, FrameResource* res)
{
	std::vector<XMFLOAT3> positions(box.Vertices.size());
	std::vector<XMFLOAT3> normals(box.Vertices.size());
	std::vector<XMFLOAT2> uvs(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		positions[i] = box.Vertices[i].Position;
		normals[i] = box.Vertices[i].Normal;
		uvs[i] = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();
	bMesh = new Mesh(
		box.Vertices.size(),
		positions.data(),
		normals.data(),
		nullptr,
		nullptr,
		uvs.data(),
		nullptr,
		nullptr,
		nullptr,
		device,
		DXGI_FORMAT_R16_UINT,
		indices.size(),
		indices.data()
	);

}