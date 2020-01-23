#include "BaseColorComponent.h"
#include "LightingComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/Shader.h"
#include "../Singleton/Graphics.h"
#include "../Singleton/PSOContainer.h"
#include "GBufferComponent.h"
#include "../LogicComponent/World.h"
#include "SkyboxComponent.h"
#include "../RenderComponent/Texture.h"
LightingComponent* lightComp;
Shader* gbufferShader;
std::unique_ptr<PSOContainer> drawRenderTargetContainer;
uint BaseColorComponent::_AllLight(0);
uint BaseColorComponent::_LightIndexBuffer(0);
uint BaseColorComponent::LightCullCBuffer(0);
uint BaseColorComponent::TextureIndices(0);

struct TextureIndices
{
	uint _UVTex;
	uint _TangentTex;
	uint _NormalTex;
	uint _DepthTex;
	uint _ShaderIDTex;
	uint _MaterialIDTex;
	uint _SkyboxCubemap;
	uint _PreintTexture;
};
const uint TEX_COUNT = sizeof(TextureIndices) / 4;
struct BaseColorFrameData : public IPipelineResource
{
	UploadBuffer texIndicesBuffer;
	uint descs[TEX_COUNT];
	BaseColorFrameData(ID3D12Device* device)
	{
		texIndicesBuffer.Create(
			device, 1, true, sizeof(TextureIndices)
		);
		World* world = World::GetInstance();
		for (uint i = 0; i < TEX_COUNT; ++i)
		{
			descs[i] = world->GetDescHeapIndexFromPool();
		}
		TextureIndices ind;
		memcpy(&ind._UVTex, descs, sizeof(uint) * TEX_COUNT);
		texIndicesBuffer.CopyData(0, &ind);
	}

	~BaseColorFrameData()
	{
		World* world = World::GetInstance();
		for (uint i = 0; i < TEX_COUNT; ++i)
		{
			world->ReturnDescHeapIndexToPool(descs[i]);
		}
	}
};

std::vector<TemporalResourceCommand>& BaseColorComponent::SendRenderTextureRequire(EventData& evt)
{
	
	auto ite = tempRTRequire.begin();
	ite->descriptor.rtDesc.width = evt.width;
	ite->descriptor.rtDesc.height = evt.height;
	return tempRTRequire;
}

struct BaseColorRunnable
{
	RenderTexture* renderTarget;
	RenderTexture* uvTex;
	RenderTexture* tangentTex;
	RenderTexture* normalTex;
	RenderTexture* depthTex;
	RenderTexture* shaderIDtex;
	RenderTexture* materialIDtex;
	ThreadCommand* threadCommand;
	Camera* cam;
	FrameResource* res;
	BaseColorComponent* selfPtr;
	ID3D12Device* device;
	static uint _GreyTex;
	static uint _IntegerTex;
	static uint _Cubemap;
	void operator()()
	{
		threadCommand->ResetCommand();
		ID3D12GraphicsCommandList* commandList = threadCommand->GetCmdList();
		BaseColorFrameData* frameData = (BaseColorFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->BaseColorFrameData*
		{
			return new BaseColorFrameData(device);
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
				selfPtr->preintContainer.get(),
				selfPtr->preintTexture->GetWidth(),
				selfPtr->preintTexture->GetHeight(),
				preintShader,
				0
			);
		}


		LightFrameData* lightFrameData = (LightFrameData*)res->GetPerCameraResource(lightComp, cam, []()->LightFrameData*
		{
#ifndef NDEBUG
			throw "No Light Data Exception!";
#endif
			return nullptr;	//Get Error if there is no light coponent in pipeline
		});
		DescriptorHeap* worldHeap = World::GetInstance()->GetGlobalDescHeap();
		uvTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[0], device);
		tangentTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[1], device);
		normalTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[2], device);
		depthTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[3], device);
		shaderIDtex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[4], device);
		materialIDtex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[5], device);
		selfPtr->skboxComp->skyboxTex->BindColorBufferToSRVHeap(worldHeap, frameData->descs[6], device);
		selfPtr->preintTexture->BindColorBufferToSRVHeap(worldHeap, frameData->descs[7], device);
		gbufferShader->BindRootSignature(commandList, worldHeap);
		ConstBufferElement ele = res->cameraCBs[cam->GetInstanceID()];
		gbufferShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), ele.buffer, ele.element);
		gbufferShader->SetResource(commandList, ShaderID::GetMainTex(), worldHeap, 0);
		gbufferShader->SetResource(commandList, _GreyTex, worldHeap, 0);
		gbufferShader->SetResource(commandList, _IntegerTex, worldHeap, 0);
		gbufferShader->SetResource(commandList, _Cubemap, worldHeap, 0);
		gbufferShader->SetStructuredBufferByAddress(commandList, BaseColorComponent::_AllLight, lightFrameData->lightsInFrustum.GetAddress(0));
		gbufferShader->SetStructuredBufferByAddress(commandList, BaseColorComponent::_LightIndexBuffer, lightComp->lightIndexBuffer->GetAddress(0, 0));
		gbufferShader->SetResource(commandList, BaseColorComponent::LightCullCBuffer, &lightFrameData->lightCBuffer, 0);
		gbufferShader->SetResource(commandList, BaseColorComponent::TextureIndices, &frameData->texIndicesBuffer, 0);

		Graphics::Blit(
			commandList,
			device,
			&renderTarget->GetColorDescriptor(0),
			1,
			&depthTex->GetColorDescriptor(0),
			drawRenderTargetContainer.get(),
			renderTarget->GetWidth(),
			renderTarget->GetHeight(),
			gbufferShader,
			0
		);
		threadCommand->CloseCommand();
	}
};

void BaseColorComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<BaseColorRunnable>(
		{
			(RenderTexture*)allTempResource[0],
			(RenderTexture*)allTempResource[1],
			(RenderTexture*)allTempResource[2],
			(RenderTexture*)allTempResource[3],
			(RenderTexture*)allTempResource[4],
			(RenderTexture*)allTempResource[5],
			(RenderTexture*)allTempResource[6],
			commandList,
			data.camera,
			data.resource,
			this,
			data.device
		});
}

uint BaseColorRunnable::_GreyTex;
uint BaseColorRunnable::_IntegerTex;
uint BaseColorRunnable::_Cubemap;
void BaseColorComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	BaseColorRunnable::_GreyTex = ShaderID::PropertyToID("_GreyTex");
	BaseColorRunnable::_IntegerTex = ShaderID::PropertyToID("_IntegerTex");
	BaseColorRunnable::_Cubemap = ShaderID::PropertyToID("_Cubemap");
	SetCPUDepending<LightingComponent>();
	SetGPUDepending<LightingComponent>();
	lightComp = RenderPipeline::GetComponent<LightingComponent>();
	tempRTRequire.resize(7);
	skboxComp = RenderPipeline::GetComponent<SkyboxComponent>();

	TextureIndices = ShaderID::PropertyToID("TextureIndices");
	tempRTRequire[0].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRTRequire[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	TemporalResourceCommand& albedoBuffer = tempRTRequire[1];
	albedoBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraUVTexture");

	TemporalResourceCommand& specularBuffer = tempRTRequire[2];
	specularBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraTangentTexture");

	TemporalResourceCommand& normalBuffer = tempRTRequire[3];
	normalBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraNormalTexture");

	TemporalResourceCommand& depthBuffer = tempRTRequire[4];
	depthBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	depthBuffer.uID = ShaderID::PropertyToID("_CameraDepthTexture");

	TemporalResourceCommand& shaderIDBuffer = tempRTRequire[5];
	shaderIDBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	shaderIDBuffer.uID = ShaderID::PropertyToID("_CameraShaderIDTexture");

	TemporalResourceCommand& materialIDBuffer = tempRTRequire[6];
	materialIDBuffer.type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	materialIDBuffer.uID = ShaderID::PropertyToID("_CameraMaterialIDTexture");

	gbufferShader = ShaderCompiler::GetShader("GBuffer");
	DXGI_FORMAT colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	drawRenderTargetContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_D32_FLOAT, 1, &colorFormat));
	_AllLight = ShaderID::PropertyToID("_AllLight");
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	LightCullCBuffer = ShaderID::PropertyToID("LightCullCBuffer");
}

void BaseColorComponent::Dispose()
{
	drawRenderTargetContainer = nullptr;
}
