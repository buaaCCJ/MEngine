#include "DepthComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/GPU Driven/GRP_Renderer.h"
#include "../RenderComponent/RenderTexture.h"
#include "../LogicComponent/World.h"
#include "PrepareComponent.h"
#include "../Singleton/PSOContainer.h"
using namespace DirectX;
PrepareComponent* prepareComp = nullptr;
PSOContainer* depthPrepassContainer(nullptr);
class DepthFrameResource : public IPipelineResource
{
public:
	UploadBuffer ub;
	UploadBuffer cullBuffer;
	DepthFrameResource(ID3D12Device* device)
	{
		cullBuffer.Create(device, 1, true, sizeof(GRP_Renderer::CullData));
		ub.Create(device, 2, true, sizeof(ObjectConstants));
		float4x4 mat = MathHelper::Identity4x4();
		XMMATRIX* vec = (XMMATRIX*)&mat;
		vec->r[3] = { 0, 0, 0, 1 };
		ub.CopyData(0, &mat);
		vec->r[3] = { 0, 0, 0, 1 };
		ub.CopyData(1, &mat);
	}
};
void DepthComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	tempRTRequire.resize(2);
	TemporalResourceCommand& depthBuffer = tempRTRequire[0];
	depthBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	depthBuffer.uID = ShaderID::PropertyToID("_CameraDepthTexture");
	depthBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_DepthBuffer;
	depthBuffer.descriptor.rtDesc.rtFormat.depthFormat = RenderTextureDepthSettings_Depth32;
	depthBuffer.descriptor.rtDesc.depthSlice = 1;
	depthBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;
	TemporalResourceCommand& motionVectorBuffer = tempRTRequire[1];
	motionVectorBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16_SNORM;
	motionVectorBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	motionVectorBuffer.descriptor.rtDesc.depthSlice = 1;
	motionVectorBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	depthPrepassContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, 0, nullptr);
}
void DepthComponent::Dispose()
{
	delete depthPrepassContainer;
}
std::vector<TemporalResourceCommand>& DepthComponent::SendRenderTextureRequire(EventData& evt)
{
	for (auto ite = tempRTRequire.begin(); ite != tempRTRequire.end(); ++ite)
	{
		ite->descriptor.rtDesc.width = evt.width;
		ite->descriptor.rtDesc.height = evt.height;
	}
	return tempRTRequire;
}

class DepthRunnable
{
public:
	RenderTexture* depthRT;
	ID3D12Device* device;		//Dx12 Device
	ThreadCommand* tcmd;		//Command List
	Camera* cam;				//Camera
	FrameResource* resource;	//Per Frame Data
	void* component;//Singleton Component
	World* world;				//Main Scene
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		DepthFrameResource* frameRes = (DepthFrameResource*)resource->GetPerCameraResource(component, cam, [=]()->DepthFrameResource*
		{
			return new DepthFrameResource(device);
		});
		depthRT->ClearRenderTarget(commandList, 0);
		depthRT->SetViewport(commandList);
		ConstBufferElement cullEle;
		cullEle.buffer = &frameRes->cullBuffer;
		cullEle.element = 0;
		//Culling
		world->grpRenderer->Culling(
			commandList,
			device,
			resource,
			cullEle,
			prepareComp->frustumPlanes,
			*(XMFLOAT3*)&prepareComp->frustumMinPos,
			*(XMFLOAT3*)&prepareComp->frustumMaxPos
		);

		//Draw Depth prepass
		commandList->OMSetRenderTargets(0, nullptr, true, &depthRT->GetColorDescriptor(0));
		world->grpRenderer->GetShader()->BindRootSignature(commandList);
		world->grpRenderer->DrawCommand(
			commandList,
			device,
			1,
			resource->cameraCBs[cam->GetInstanceID()],
			depthPrepassContainer
		);
		tcmd->CloseCommand();
	}
};

void DepthComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<DepthRunnable>({
		(RenderTexture*)allTempResource[0],
		data.device,
		commandList,
		data.camera,
		data.resource,
		this,
		data.world
	});
}