#include "GBufferComponent.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/StructuredBuffer.h"
#include "../RenderComponent/MeshRenderer.h"
#include "RenderPipeline.h"
#include "../LogicComponent/Transform.h"
#include "../Common/GeometryGenerator.h"
#include "../RenderComponent/GPU Driven/GRP_Renderer.h"
#include "../Singleton/MeshLayout.h"
#include "PrepareComponent.h"
#include "LightingComponent.h"
#include "../RenderComponent/Light.h"
#include "SkyboxComponent.h"
#include "../RenderComponent/Texture.h"
using namespace DirectX;
PSOContainer* gbufferContainer(nullptr);

LightingComponent* lightComp;
uint GBufferComponent::_AllLight(0);
uint GBufferComponent::_LightIndexBuffer(0);
uint GBufferComponent::LightCullCBuffer(0);
uint GBufferComponent::TextureIndices(0);

struct TextureIndices
{
	uint _SkyboxCubemap;
	uint _PreintTexture;
};
const uint TEX_COUNT = sizeof(TextureIndices) / 4;

RenderTexture* gbufferTempRT[10];

#define ALBEDO_RT (gbufferTempRT[0])
#define SPECULAR_RT  (gbufferTempRT[1])
#define NORMAL_RT  (gbufferTempRT[2])
#define MOTION_VECTOR_RT (gbufferTempRT[3])
#define EMISSION_RT (gbufferTempRT[4])
#define DEPTH_RT (gbufferTempRT[5])
#define GBUFFER_COUNT 5


struct GBufferFrameData : public IPipelineResource
{
	UploadBuffer texIndicesBuffer;
	uint descs[TEX_COUNT];
	GBufferFrameData(ID3D12Device* device) : 
		texIndicesBuffer(
			device, 1, true, sizeof(TextureIndices)
		)
	{
		World* world = World::GetInstance();
		for (uint i = 0; i < TEX_COUNT; ++i)
		{
			descs[i] = world->GetDescHeapIndexFromPool();
		}
		TextureIndices ind;
		memcpy(&ind, descs, sizeof(uint) * TEX_COUNT);
		texIndicesBuffer.CopyData(0, &ind);
	}

	~GBufferFrameData()
	{
		World* world = World::GetInstance();
		for (uint i = 0; i < TEX_COUNT; ++i)
		{
			world->ReturnDescHeapIndexToPool(descs[i]);
		}
	}
};

class GBufferRunnable
{
public:
	ID3D12Device* device;		//Dx12 Device
	ThreadCommand* tcmd;		//Command List
	Camera* cam;				//Camera
	FrameResource* resource;	//Per Frame Data
	GBufferComponent* selfPtr;//Singleton Component
	World* world;				//Main Scene
	static uint _GreyTex;
	static uint _IntegerTex;
	static uint _Cubemap;
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		//Clear
		
		GBufferFrameData* frameData = (GBufferFrameData*)resource->GetPerCameraResource(selfPtr, cam, [&]()->GBufferFrameData*
		{
			return new GBufferFrameData(device);
		});
		if (!selfPtr->preintTexture)
		{
			RenderTextureFormat format;
			format.colorFormat = DXGI_FORMAT_R16G16_UNORM;
			selfPtr->preintContainer = std::unique_ptr<PSOContainer>(new PSOContainer(
				DXGI_FORMAT_UNKNOWN,
				1,
				&format.colorFormat
			));

			format.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
			selfPtr->preintTexture = new RenderTexture(
				device,
				256,
				256,
				format,
				RenderTextureDimension_Tex2D,
				1,
				1);
			Shader* preintShader = ShaderCompiler::GetShader("PreInt");
			preintShader->BindRootSignature(commandList);
			Graphics::Blit(
				commandList,
				device,
				&selfPtr->preintTexture->GetColorDescriptor(0),
				1,
				nullptr,
				selfPtr->preintContainer.get(), 0,
				selfPtr->preintTexture->GetWidth(),
				selfPtr->preintTexture->GetHeight(),
				preintShader,
				0
			);
		}


		LightFrameData* lightFrameData = (LightFrameData*)resource->GetPerCameraResource(lightComp, cam, []()->LightFrameData*
		{
#ifndef NDEBUG
			throw "No Light Data Exception!";
#endif
			return nullptr;	//Get Error if there is no light coponent in pipeline
		});
		DescriptorHeap* worldHeap = World::GetInstance()->GetGlobalDescHeap();
		selfPtr->skboxComp->skyboxTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[0], device);
		selfPtr->preintTexture->BindColorBufferToSRVHeap(worldHeap, frameData->descs[1], device);
		Shader* gbufferShader = world->grpRenderer->GetShader();
		gbufferShader->BindRootSignature(commandList, worldHeap);
		gbufferShader->SetResource(commandList, ShaderID::GetMainTex(), worldHeap, 0);
		gbufferShader->SetResource(commandList, _GreyTex, worldHeap, 0);
		gbufferShader->SetResource(commandList, _IntegerTex, worldHeap, 0);
		gbufferShader->SetResource(commandList, _Cubemap, worldHeap, 0);
		gbufferShader->SetStructuredBufferByAddress(commandList, GBufferComponent::_AllLight, lightFrameData->lightsInFrustum.GetAddress(0));
		gbufferShader->SetStructuredBufferByAddress(commandList, GBufferComponent::_LightIndexBuffer, lightComp->lightIndexBuffer->GetAddress(0, 0));
		gbufferShader->SetResource(commandList, GBufferComponent::LightCullCBuffer, &lightFrameData->lightCBuffer, 0);
		gbufferShader->SetResource(commandList, GBufferComponent::TextureIndices, &frameData->texIndicesBuffer, 0);

		MOTION_VECTOR_RT->ClearRenderTarget(commandList, 0);
		//Prepare RenderTarget
		D3D12_CPU_DESCRIPTOR_HANDLE handles[GBUFFER_COUNT];
		auto st = [&](UINT p)->void
		{
			handles[p] = gbufferTempRT[p]->GetColorDescriptor(0);
		};

		InnerLoop<decltype(st), GBUFFER_COUNT>(st);
		EMISSION_RT->SetViewport(commandList);
		
		//Draw GBuffer
		commandList->OMSetRenderTargets(GBUFFER_COUNT, handles, false, &DEPTH_RT->GetColorDescriptor(0));
		world->grpRenderer->DrawCommand(
			commandList,
			device,
			0,
			resource->cameraCBs[cam->GetInstanceID()],
			gbufferContainer, 0
		);


		tcmd->CloseCommand();
	}
};
uint GBufferRunnable::_GreyTex;
uint GBufferRunnable::_IntegerTex;
uint GBufferRunnable::_Cubemap;
std::vector<TemporalResourceCommand>& GBufferComponent::SendRenderTextureRequire(EventData& evt) {
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		tempRTRequire[i].descriptor.rtDesc.width = evt.width;
		tempRTRequire[i].descriptor.rtDesc.height = evt.height;
	}
	return tempRTRequire;
}
void GBufferComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	memcpy(gbufferTempRT, allTempResource.data(), sizeof(RenderTexture*) * tempRTRequire.size());
	GBufferRunnable runnable
	{
		data.device,
		commandList,
		data.camera,
		data.resource,
		this,
		data.world
	};
	//Schedule MultiThread Job
	ScheduleJob(runnable);
}

void GBufferComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<LightingComponent>();
	SetGPUDepending<LightingComponent>();
	lightComp = RenderPipeline::GetComponent<LightingComponent>();
	tempRTRequire.resize(6);
	GBufferRunnable::_GreyTex = ShaderID::PropertyToID("_GreyTex");
	GBufferRunnable::_IntegerTex = ShaderID::PropertyToID("_IntegerTex");
	GBufferRunnable::_Cubemap = ShaderID::PropertyToID("_Cubemap");
	TemporalResourceCommand& specularBuffer = tempRTRequire[0];
	specularBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	specularBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	specularBuffer.descriptor.rtDesc.depthSlice = 1;
	specularBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;

	TemporalResourceCommand& albedoBuffer = tempRTRequire[1];
	albedoBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	albedoBuffer.descriptor.rtDesc.depthSlice = 1;
	albedoBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;

	TemporalResourceCommand& normalBuffer = tempRTRequire[2];
	normalBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	normalBuffer.descriptor.rtDesc.depthSlice = 1;
	normalBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;

	TemporalResourceCommand& motionVectorBuffer = tempRTRequire[3];
	motionVectorBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16_SNORM;

	tempRTRequire[4].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRTRequire[4].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRTRequire[4].descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	TemporalResourceCommand& depthBuffer = tempRTRequire[5];
	depthBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	depthBuffer.uID = ShaderID::PropertyToID("_CameraDepthTexture");

	std::vector<DXGI_FORMAT> colorFormats(GBUFFER_COUNT);
	for (int i = 0; i < GBUFFER_COUNT; ++i)
	{
		colorFormats[i] = tempRTRequire[i].descriptor.rtDesc.rtFormat.colorFormat;
	}
	gbufferContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, GBUFFER_COUNT, colorFormats.data());
	_AllLight = ShaderID::PropertyToID("_AllLight");
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	LightCullCBuffer = ShaderID::PropertyToID("LightCullCBuffer");
	TextureIndices = ShaderID::PropertyToID("TextureIndices");
	skboxComp = RenderPipeline::GetComponent<SkyboxComponent>();
}

void GBufferComponent::Dispose()
{

	//meshRenderer->Destroy();
	//trans.Destroy();
	delete gbufferContainer;

}
