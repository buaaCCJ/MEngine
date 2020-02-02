#include "PostProcessingComponent.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "PrepareComponent.h"
//#include "TemporalAA.h"
#include "../RenderComponent/Texture.h"
#include "PostProcess/ColorGradingLut.h"
#include "PostProcess//LensDistortion.h"
#include "RenderPipeline.h"
//#include "SkyboxComponent.h"
Shader* postShader;
std::unique_ptr<PSOContainer> backBufferContainer;
int _Lut3D;
//std::unique_ptr<TemporalAA> taaComponent;
std::unique_ptr<ColorGradingLut> lutComponent;
//ObjectPtr<Texture> testTex;
//PrepareComponent* prepareComp = nullptr;
struct PostParams
{
	float4 _Lut3DParam;
	float4 _Distortion_Amount;
	float4 _Distortion_CenterScale;
	float4 _MainTex_TexelSize;
	float _ChromaticAberration_Amount;
};
class PostFrameData : public IPipelineResource
{
public:
	DescriptorHeap postSRVHeap;
	UploadBuffer postUBuffer;
	PostFrameData(ID3D12Device* device)
		: postSRVHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true),
		postUBuffer(device, 1, true, sizeof(PostParams))
	{
	}
};
uint Params = 0;
class PostRunnable
{
public:
	RenderTexture* renderTarget;
	RenderTexture* depthTarget;
	RenderTexture* motionVector;
	RenderTexture* destMap;
	ThreadCommand* threadCmd;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	UINT width;
	UINT height;
	void* selfPtr;
	FrameResource* resource;
	bool isForPresent;
	Camera* cam;
	void operator()()
	{
		threadCmd->ResetCommand();

		ID3D12GraphicsCommandList* commandList = threadCmd->GetCmdList();
		//		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ, motionVector->GetColorResource());
		PostFrameData* frameRes = (PostFrameData*)resource->GetPerCameraResource(selfPtr, cam,
			[=]()->PostFrameData*
		{
			return new PostFrameData(device);
		});

		/*taaComponent->Run(
			renderTarget,
			depthTarget,
			motionVector,
			destMap,
			commandList,
			resource,
			cam,
			width, height
		);*/
		(*lutComponent)(
			device,
			commandList);
		lutComponent->lut->BindColorBufferToSRVHeap(&frameRes->postSRVHeap, 1, device);
		destMap->BindColorBufferToSRVHeap(&frameRes->postSRVHeap, 0, device);
		if (isForPresent)
		{
			Graphics::ResourceStateTransform(
				commandList,
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				backBuffer);
		}
		postShader->BindRootSignature(commandList, &frameRes->postSRVHeap);
		PostParams& params = *(PostParams*)frameRes->postUBuffer.GetMappedDataPtr(0);
		params._Lut3DParam = { 1.0f / lutComponent->k_Lut3DSize, lutComponent->k_Lut3DSize - 1.0f, 1, 1 };
		params._MainTex_TexelSize = {
			1.0f / width,
			1.0f/ height,
			(float)width,
			(float)height
		};
		GetLensDistortion(
			params._Distortion_CenterScale,
			params._Distortion_Amount,
			params._ChromaticAberration_Amount);
		postShader->SetResource(commandList, ShaderID::GetMainTex(), &frameRes->postSRVHeap, 0);
		postShader->SetResource(commandList, _Lut3D, &frameRes->postSRVHeap, 1);
		postShader->SetResource(commandList, Params, &frameRes->postUBuffer, 0);
		Graphics::Blit(
			commandList,
			device,
			&backBufferHandle,
			1,
			nullptr,
			backBufferContainer.get(),
			width, height,
			postShader,
			0
		);
		if (isForPresent) {
			Graphics::ResourceStateTransform(
				commandList,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT,
				backBuffer);
		}
		//	Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET, motionVector->GetColorResource());
		threadCmd->CloseCommand();
	}
};

void PostProcessingComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	JobHandle handle = ScheduleJob<PostRunnable>({
		(RenderTexture*)allTempResource[0],
		(RenderTexture*)allTempResource[3],
		(RenderTexture*)allTempResource[1],
		(RenderTexture*)allTempResource[2],
		commandList,
		data.backBufferHandle,
		data.backBuffer,
		data.device,
		data.width,
		data.height,
		this,
		data.resource,
		data.isBackBufferForPresent,
		data.camera
		});
}
void PostProcessingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	_Lut3D = ShaderID::PropertyToID("_Lut3D");
	Params = ShaderID::PropertyToID("Params");
	SetCPUDepending<PrepareComponent>();
	//	SetGPUDepending<SkyboxComponent>();
	tempRT.resize(4);
	tempRT[0].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_PostProcessBlitTarget");
	tempRT[3].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[3].uID = ShaderID::PropertyToID("_CameraDepthTexture");


	postShader = ShaderCompiler::GetShader("PostProcess");
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &backBufferFormat)
		);

	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	/*
	taaComponent = std::unique_ptr<TemporalAA>(new TemporalAA());
	taaComponent->prePareComp = prepareComp;
	taaComponent->device = device;
	taaComponent->toRTContainer = renderTextureContainer.get();*/
	lutComponent = std::unique_ptr<ColorGradingLut>(new ColorGradingLut(device));
	/*	testTex = new Texture(
			device,
			"Test",
			L"Resource/Test.vtex"
		);*/
}

std::vector<TemporalResourceCommand>& PostProcessingComponent::SendRenderTextureRequire(EventData& evt)
{
	auto& desc = tempRT[2].descriptor;
	desc.rtDesc.width = evt.width;
	desc.rtDesc.height = evt.height;
	return tempRT;
}

void PostProcessingComponent::Dispose()
{
	backBufferContainer = nullptr;
	//taaComponent = nullptr;
	lutComponent = nullptr;
	//testTex.Destroy();
}