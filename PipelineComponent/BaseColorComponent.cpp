#include "BaseColorComponent.h"
#include "LightingComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/Shader.h"
#include "../Singleton/Graphics.h"
#include "../Singleton/PSOContainer.h"
#include "GBufferComponent.h"
#include "../LogicComponent/World.h"
LightingComponent* lightComp;
Shader* gbufferShader;
std::unique_ptr<PSOContainer> drawRenderTargetContainer;
UINT BaseColorComponent::_AllLight(0);
UINT BaseColorComponent::_LightIndexBuffer(0);
UINT BaseColorComponent::LightCullCBuffer(0);
UINT BaseColorComponent::TextureIndices(0);

void BaseColorComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<LightingComponent>();
	SetGPUDepending<LightingComponent>();
	lightComp = RenderPipeline::GetComponent<LightingComponent>();
	tempRTRequire.resize(7);


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


struct TextureIndices
{
	UINT _UVTex;
	UINT _TangentTex;
	UINT _NormalTex;
	UINT _DepthTex;
	UINT _ShaderIDTex;
	UINT _MaterialIDTex;
};
#define TEX_COUNT 6
struct BaseColorFrameData : public IPipelineResource
{
	UploadBuffer texIndicesBuffer;
	UINT descs[TEX_COUNT];
	BaseColorFrameData(ID3D12Device* device)
	{
		texIndicesBuffer.Create(
			device, 1, true, sizeof(TextureIndices)
		);
		World* world = World::GetInstance();
		auto func = [&](UINT i)->void {
			descs[i] = world->GetDescHeapIndexFromPool();
		};
		InnerLoop<decltype(func), TEX_COUNT>(func);
		TextureIndices ind;
		memcpy(&ind._UVTex, descs, sizeof(UINT) * TEX_COUNT);
		texIndicesBuffer.CopyData(0, &ind);
	}

	~BaseColorFrameData()
	{
		World* world = World::GetInstance();
		auto func = [&](UINT i)->void
		{
			world->ReturnDescHeapIndexToPool(descs[i]);
		};
		InnerLoop<decltype(func), TEX_COUNT>(func);
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
	void* selfPtr;
	ID3D12Device* device;
	void operator()()
	{
		threadCommand->ResetCommand();
		ID3D12GraphicsCommandList* commandList = threadCommand->GetCmdList();
		BaseColorFrameData* frameData = (BaseColorFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->BaseColorFrameData*
		{
			return new BaseColorFrameData(device);
		});
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
		gbufferShader->BindRootSignature(commandList, worldHeap);
		ConstBufferElement ele = res->cameraCBs[cam->GetInstanceID()];
		gbufferShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), ele.buffer, ele.element);
		gbufferShader->SetResource(commandList, ShaderID::GetMainTex(), worldHeap, 0);
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