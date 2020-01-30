#include "GRP_Renderer.h"
#include "../DescriptorHeap.h"
#include "../Mesh.h"
#include "../../LogicComponent/Transform.h"
#include "../../Singleton/ShaderCompiler.h"
#include "../ComputeShader.h"
#include "../../Singleton/FrameResource.h"
#include "../StructuredBuffer.h"
#include "../../Common/MetaLib.h"
#include "../../Singleton/ShaderID.h"
#include "../../Singleton/PSOContainer.h"
#include <mutex>
#include "../../PipelineComponent/IPerCameraResource.h"
#include "../../Singleton/Graphics.h"
struct ObjectData
{
	DirectX::XMFLOAT4X4 localToWorld;
	DirectX::XMFLOAT3 boundingCenter;
	DirectX::XMFLOAT3 boundingExtent;
	DirectX::XMUINT2 id;
};

struct Command
{
	enum CommandType
	{
		CommandType_Add,
		CommandType_Remove,
		CommandType_TransformPos,
		CommandType_Renderer
	};
	CommandType type;
	UINT index;
	MultiDrawCommand cmd;
	ObjectData objData;
	Command() {}
	Command(const Command& cmd)
	{
		memcpy(this, &cmd, sizeof(Command));
	}
};
class GpuDrivenRenderer : public IPipelineResource
{
public:
	std::unique_ptr<UploadBuffer> objectPosBuffer;
	std::unique_ptr<UploadBuffer> cmdDrawBuffers;
	CBufferPool objectPool;
	std::mutex mtx;
	UINT capacity;
	UINT count;
	std::vector<Command> allCommands;
	std::vector<ConstBufferElement> allocatedObjects;
	GpuDrivenRenderer(
		ID3D12Device* device,
		UINT capacity
	) : capacity(capacity), count(0),
		objectPool(sizeof(ObjectConstants), capacity),
		objectPosBuffer(new UploadBuffer(device, capacity, false, sizeof(ObjectData))),
		cmdDrawBuffers(new UploadBuffer(device, capacity, false, sizeof(MultiDrawCommand)))
	{
		allocatedObjects.reserve(capacity);
		
	}

	void Resize(
		UINT targetCapacity,
		ID3D12Device* device)
	{
		if (targetCapacity <= capacity) return;
		UINT autoCapac = (UINT)(capacity * 1.5);
		targetCapacity = max(targetCapacity, autoCapac);
		UploadBuffer* newObjBuffer = new UploadBuffer(device, targetCapacity, false, sizeof(ObjectData));
		UploadBuffer* newCmdDrawBuffer = new UploadBuffer(device, targetCapacity, false, sizeof(MultiDrawCommand));
		newObjBuffer->CopyFrom(objectPosBuffer.get(), 0, 0, capacity);
		newCmdDrawBuffer->CopyFrom(cmdDrawBuffers.get(), 0, 0, capacity);

		objectPosBuffer = std::unique_ptr<UploadBuffer>(newObjBuffer);
		cmdDrawBuffers = std::unique_ptr<UploadBuffer>(newCmdDrawBuffer);
		capacity = targetCapacity;
	}

	void AddCommand(Command& cmd)
	{
		std::lock_guard<std::mutex> lck(mtx);
		allCommands.push_back(cmd);
	}

	void UpdateFrame(
		ID3D12Device* device
	)
	{
		for (UINT i = 0; ; ++i)
		{
			Command c;
			mtx.lock();
			if (i >= allCommands.size())
			{
				goto END_OF_LOOP;
			}
			c = allCommands[i];
			mtx.unlock();
			ConstBufferElement cEle;// = allocatedObjects[c.index];
			std::vector<ConstBufferElement>::iterator ite;// = allocatedObjects.end() - 1;
			UINT last = count - 1;
			switch (c.type)
			{
			case Command::CommandType_Add:
			{
				Resize(count + 1, device);
				ConstBufferElement obj = objectPool.Get(device);
				c.cmd.objectCBufferAddress = obj.buffer->GetAddress(obj.element);
				allocatedObjects.push_back(obj);
				ConstBufferElement objEle = allocatedObjects[allocatedObjects.size() - 1];
				ObjectConstants* mappedPtr = (ObjectConstants*)objEle.buffer->GetMappedDataPtr(objEle.element);
				mappedPtr->id = c.objData.id;
				mappedPtr->lastObjectToWorld = c.objData.localToWorld;
				mappedPtr->objectToWorld = c.objData.localToWorld;
				objectPosBuffer->CopyData(count, &c.objData);
				cmdDrawBuffers->CopyData(count, &c.cmd);
				count++;
			}
				break;
			case Command::CommandType_Remove:
				if (c.index != last)
				{
					objectPosBuffer->CopyDataInside(last, c.index);
					cmdDrawBuffers->CopyDataInside(last, c.index);
				}
				cEle = allocatedObjects[c.index];
				objectPool.Return(cEle);
				ite = allocatedObjects.end() - 1;
				cEle = *ite;
				allocatedObjects.erase(ite);
				count--;
				break;
			case Command::CommandType::CommandType_TransformPos:
			{
				ConstBufferElement objEle = allocatedObjects[c.index];
				ObjectConstants* mappedPtr = (ObjectConstants*)objEle.buffer->GetMappedDataPtr(objEle.element);
				mappedPtr->lastObjectToWorld = mappedPtr->objectToWorld;
				mappedPtr->objectToWorld = c.objData.localToWorld;
				mappedPtr->id = c.objData.id;
				objectPosBuffer->CopyData(c.index, &c.objData);
			}
				break;
			case Command::CommandType::CommandType_Renderer:
				cEle = allocatedObjects[c.index];
				c.cmd.objectCBufferAddress = cEle.buffer->GetAddress(cEle.element); 
				cmdDrawBuffers->CopyData(c.index, &c.cmd);
				break;
			}
		}
	END_OF_LOOP:
		allCommands.clear();
		mtx.unlock();
	}

	~GpuDrivenRenderer()
	{
	}
};
GRP_Renderer::GRP_Renderer(
	UINT meshLayoutIndex,
	UINT initCapacity,
	Shader* shader,
	ID3D12Device* device
) :
	MObject(),
	capacity(initCapacity),
	shader(shader),
	cmdSig(shader, device),
	meshLayoutIndex(meshLayoutIndex)
{
	cullShader = ShaderCompiler::GetComputeShader("Cull");
	StructuredBufferElement ele[2];
	ele[0].elementCount = capacity;
	ele[0].stride = sizeof(MultiDrawCommand);
	ele[1].elementCount = 1;
	ele[1].stride = sizeof(UINT);
	cullResultBuffer = std::unique_ptr<StructuredBuffer>(new StructuredBuffer(
		device,
		ele,
		2
	));


	_InputBuffer = ShaderID::PropertyToID("_InputBuffer");
	_InputDataBuffer = ShaderID::PropertyToID("_InputDataBuffer");
	_OutputBuffer = ShaderID::PropertyToID("_OutputBuffer");
	_CountBuffer = ShaderID::PropertyToID("_CountBuffer");
	CullBuffer = ShaderID::PropertyToID("CullBuffer");
}

GRP_Renderer::RenderElement& GRP_Renderer::AddRenderElement(
	ObjectPtr<Transform>& targetTrans,
	ObjectPtr<Mesh>& mesh,
	ID3D12Device* device,
	UINT shaderID,
	UINT materialID
)
{
	if (mesh->GetLayoutIndex() != meshLayoutIndex)
		throw "Mesh Bad Layout!";
	auto&& ite = dicts.find(targetTrans.operator->());
	if (ite != dicts.end())
	{
		return elements[ite->second];
	}
	dicts.insert_or_assign(targetTrans.operator->(), elements.size());
	
	RenderElement& ele = elements.emplace_back(
		&targetTrans,
		mesh->boundingCenter,
		mesh->boundingExtent
	);

	Command cmd;
	cmd.index = elements.size() - 1;
	cmd.type = Command::CommandType_Add;
	cmd.cmd.objectCBufferAddress = 0;
	cmd.cmd.vertexBuffer = mesh->VertexBufferView();
	cmd.cmd.indexBuffer = mesh->IndexBufferView();
	cmd.cmd.drawArgs.BaseVertexLocation = 0;
	cmd.cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
	cmd.cmd.drawArgs.InstanceCount = 1;
	cmd.cmd.drawArgs.StartIndexLocation = 0;
	cmd.cmd.drawArgs.StartInstanceLocation = 0;
	cmd.objData.boundingCenter = ele.boundingCenter;
	cmd.objData.boundingExtent = ele.boundingExtent;
	cmd.objData.id = { shaderID, materialID };
	DirectX::XMMATRIX* ptr = (DirectX::XMMATRIX*) &cmd.objData.localToWorld;
	*ptr = targetTrans->GetLocalToWorldMatrix();
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource( this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}

	return ele;
}

void GRP_Renderer::UpdateRenderer(Transform* targetTrans, Mesh* mesh, ID3D12Device* device)
{
	auto&& ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];

	Command cmd;
	cmd.index = ite->second;
	cmd.type = Command::CommandType_Renderer;
	cmd.cmd.objectCBufferAddress = 0;
	cmd.cmd.vertexBuffer = mesh->VertexBufferView();
	cmd.cmd.indexBuffer = mesh->IndexBufferView();
	cmd.cmd.drawArgs.BaseVertexLocation = 0;
	cmd.cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
	cmd.cmd.drawArgs.InstanceCount = 1;
	cmd.cmd.drawArgs.StartIndexLocation = 0;
	cmd.cmd.drawArgs.StartInstanceLocation = 0;
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}
}

void GRP_Renderer::RemoveElement(Transform* trans, ID3D12Device* device)
{
	auto&& ite = dicts.find(trans);
	if (ite == dicts.end()) return;
	auto arrEnd = elements.end() - 1;
	UINT index = ite->second;
	RenderElement& ele = elements[ite->second];
	ele = *arrEnd;
	dicts.insert_or_assign(arrEnd->transform.operator->(), ite->second);
	elements.erase(arrEnd);
	dicts.erase(ite);

	Command cmd;
	cmd.index = index;
	cmd.type = Command::CommandType_Remove;
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}

}

void GRP_Renderer::UpdateFrame(FrameResource* resource, ID3D12Device* device)
{
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)resource->GetResource(this, [=]()->GpuDrivenRenderer*
	{
		return new GpuDrivenRenderer(device, capacity);
	});
	perFrameData->UpdateFrame(device);
}

void GRP_Renderer::UpdateTransform(
	Transform* targetTrans,
	ID3D12Device* device,
	UINT shaderID,
	UINT materialID)
{
	auto&& ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];
	Command cmd;
	cmd.index = ite->second;
	cmd.type = Command::CommandType_TransformPos;
	cmd.objData.boundingCenter = rem.boundingCenter;
	cmd.objData.boundingExtent = rem.boundingExtent;
	cmd.objData.id = { shaderID, materialID };
	DirectX::XMMATRIX* ptr = (DirectX::XMMATRIX*) &cmd.objData.localToWorld;
	*ptr = targetTrans->GetLocalToWorldMatrix();
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}
}
void GRP_Renderer::Culling(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	FrameResource* targetResource,
	ConstBufferElement& cullDataBuffer,
	DirectX::XMFLOAT4* frustumPlanes,
	DirectX::XMFLOAT3 frustumMinPoint,
	DirectX::XMFLOAT3 frustumMaxPoint)
{
	if (elements.size() > cullResultBuffer->GetElementCount(0))
	{
		cullResultBuffer->ReleaseResourceAfterFlush(targetResource);
		StructuredBufferElement ele[2];
		ele[0].elementCount = elements.size();
		ele[0].stride = sizeof(MultiDrawCommand);
		ele[1].elementCount = 1;
		ele[1].stride = sizeof(UINT);
		cullResultBuffer = std::unique_ptr<StructuredBuffer>(
			new StructuredBuffer(
				device,
				ele,
				2
			));
		isIndirect = false;

	}
	if (!isIndirect)
	{
		cullResultBuffer->TransformBufferState(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		isIndirect = true;
	}
	cullShader->BindRootSignature(commandList, nullptr);
	UINT dispatchCount = (UINT)ceil(elements.size() / 64.0);
	UINT capacity = this->capacity;
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)targetResource->GetResource(this, [=]()->GpuDrivenRenderer*
	{
		return new GpuDrivenRenderer(device, capacity);
	});
	CullData cullD;
	memcpy(cullD.planes, frustumPlanes, sizeof(DirectX::XMFLOAT4) * 6);
	memcpy(&cullD._FrustumMaxPoint, &frustumMaxPoint, sizeof(DirectX::XMFLOAT3));
	memcpy(&cullD._FrustumMinPoint, &frustumMinPoint, sizeof(DirectX::XMFLOAT3));
	cullD._Count = elements.size();
	cullDataBuffer.buffer->CopyData(cullDataBuffer.element, &cullD);
	cullShader->SetResource(commandList, _InputBuffer, perFrameData->cmdDrawBuffers.get(), 0);
	cullShader->SetResource(commandList, _InputDataBuffer, perFrameData->objectPosBuffer.get(), 0);
	cullShader->SetStructuredBufferByAddress(commandList, _OutputBuffer, cullResultBuffer->GetAddress(0, 0));
	cullShader->SetStructuredBufferByAddress(commandList, _CountBuffer, cullResultBuffer->GetAddress(1, 0));
	cullShader->SetResource(commandList, CullBuffer, cullDataBuffer.buffer, cullDataBuffer.element);
	cullResultBuffer->TransformBufferState(commandList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cullShader->Dispatch(commandList, 1, 1, 1, 1);
	cullShader->Dispatch(commandList, 0, dispatchCount, 1, 1);
	cullResultBuffer->TransformBufferState(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
}
void  GRP_Renderer::DrawCommand(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	UINT targetShaderPass,
	ConstBufferElement& cameraProperty,
	PSOContainer* container
)
{
	PSODescriptor desc;
	desc.meshLayoutIndex = meshLayoutIndex;
	desc.shaderPass = targetShaderPass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	shader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraProperty.buffer, cameraProperty.element);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->ExecuteIndirect(
		cmdSig.GetSignature(),
		elements.size(),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(0, 0),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(1, 0)
	);

}

CBufferPool* GRP_Renderer::GetCullDataPool(UINT initCapacity)
{
	return new CBufferPool(sizeof(CullData), initCapacity);
}

GRP_Renderer::~GRP_Renderer()
{
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		FrameResource::mFrameResources[i]->DisposeResource(this);
	}
}
