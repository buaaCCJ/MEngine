#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "Mesh.h"
#include "CBufferPool.h"
#include "../LogicComponent/Component.h"
class PSOContainer;
class Shader;
class UploadBuffer;
struct MultiDrawCommand;

class MeshRenderer : public Component
{
public:
	struct MeshRendererObjectData
	{
		DirectX::XMMATRIX localToWorld;
		DirectX::XMFLOAT3 boundingCenter;
		DirectX::XMFLOAT3 boundingExtent;
	};
	static std::vector<std::pair<MeshRenderer*, MeshRendererObjectData>> allRendererData;
	virtual ~MeshRenderer();
	ObjectPtr<Mesh> mesh;
	UINT listIndex;
	Shader* mShader;
	MeshRenderer(
		Transform* trans,
		ID3D12Device* device,
		ObjectPtr<Mesh>& initMesh,
		Shader* shader
	);
	void Draw(
		int targetPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		UploadBuffer* objectBuffer,
		UINT objectBufferOffset,
		PSOContainer* container
	);
};