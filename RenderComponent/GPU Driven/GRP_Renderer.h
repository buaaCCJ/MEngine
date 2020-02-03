#pragma once
#include "../../Common/MObject.h"
#include "../CommandSignature.h"
#include "../../Common/MetaLib.h"
#include "../CBufferPool.h"
class DescriptorHeap;
class Mesh;
class Transform;
class FrameResource;
class ComputeShader;
class PSOContainer;
class StructuredBuffer;
class GRP_Renderer : public MObject
{
public:
	struct CullData
	{
		DirectX::XMFLOAT4 planes[6];
		//Align
		DirectX::XMFLOAT3 _FrustumMinPoint;
		UINT _Count;
		//Align
		DirectX::XMFLOAT3 _FrustumMaxPoint;
	};
	struct RenderElement
	{
		ObjectPtr<Transform> transform;
		DirectX::XMFLOAT3 boundingCenter;
		DirectX::XMFLOAT3 boundingExtent;
		RenderElement(
			ObjectPtr<Transform>* anotherTrans,
			DirectX::XMFLOAT3 boundingCenter,
			DirectX::XMFLOAT3 boundingExtent
		) : transform(*anotherTrans), boundingCenter(boundingCenter), boundingExtent(boundingExtent) {}
	};
private:
	bool isIndirect = false;
	CommandSignature cmdSig;
	Shader* shader;
	std::unique_ptr<StructuredBuffer> cullResultBuffer;
	std::vector<RenderElement> elements;
	std::unordered_map<Transform*, UINT> dicts;
	UINT meshLayoutIndex;
	UINT capacity;
	UINT _InputBuffer;
	UINT _InputDataBuffer;
	UINT _OutputBuffer;
	UINT _CountBuffer;
	UINT CullBuffer;
	ComputeShader* cullShader;
public:
	GRP_Renderer(
		UINT meshLayoutIndex,
		UINT initCapacity,
		Shader* shader,
		ID3D12Device* device
	);
	~GRP_Renderer();
	RenderElement& AddRenderElement(
		ObjectPtr<Transform>& targetTrans,
		ObjectPtr<Mesh>& mesh,
		ID3D12Device* device,
		UINT shaderID,
		UINT materialID
	);
	inline Shader* GetShader() const { return shader; }
	static CBufferPool* GetCullDataPool(UINT initCapacity);
	void RemoveElement(Transform* trans, ID3D12Device* device);
	void UpdateRenderer(Transform* targetTrans, Mesh* mesh, ID3D12Device* device);
	CommandSignature* GetCmdSignature() { return &cmdSig; }
	void UpdateFrame(FrameResource*, ID3D12Device*);//Should be called Per frame
	void UpdateTransform(
		Transform* targetTrans,
		ID3D12Device* device,
		UINT shaderID,
		UINT materialID);
	void Culling(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		FrameResource* targetResource,
		ConstBufferElement& cullDataBuffer,
		DirectX::XMFLOAT4* frustumPlanes,
		DirectX::XMFLOAT3 frustumMinPoint,
		DirectX::XMFLOAT3 frustumMaxPoint
	);
	void DrawCommand(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		UINT targetShaderPass,
		ConstBufferElement& cameraProperty,
		PSOContainer* container, uint containerIndex
	);
};
