#include "TemporalAntiAlias.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "CameraData/CameraTransformData.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/RenderTexture.h"
#include "PrepareComponent.h"
#include "RenderPipeline.h"
using namespace DirectX;		//Only for single .cpp, namespace allowed
struct TAAConstBuffer
{
	XMFLOAT4X4 _InvNonJitterVP;
	XMFLOAT4X4 _InvLastVp;
	XMFLOAT4 _FinalBlendParameters;
	XMFLOAT4 _CameraDepthTexture_TexelSize;
	XMFLOAT4 _ScreenParams;		//x is the width of the camera¡¯s target texture in pixels, y is the height of the camera¡¯s target texture in pixels, z is 1.0 + 1.0 / width and w is 1.0 + 1.0 / height.
	XMFLOAT4 _ZBufferParams;		// x is (1-far/near), y is (far/near), z is (x/far) and w is (y/far).
	//Align
	XMFLOAT3 _TemporalClipBounding;
	float _Sharpness;
	//Align
	XMFLOAT2 _Jitter;
	XMFLOAT2 _LastJitter;

};


std::unique_ptr<PSOContainer> renderTextureContainer;

struct TAAFrameData : public IPipelineResource
{
	UploadBuffer taaBuffer;
	DescriptorHeap srvHeap;
	TAAFrameData(ID3D12Device* device) :
		taaBuffer(device, 1, true, sizeof(TAAConstBuffer)),
		srvHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true)
	{
	}
};

struct TAACameraData : public IPipelineResource
{
	std::unique_ptr<RenderTexture> lastRenderTarget;
	std::unique_ptr<RenderTexture> lastDepthTexture;
	std::unique_ptr<RenderTexture> lastMotionVectorTexture;
	UINT width, height;
private:
	void Update(UINT width, UINT height, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
	{
		this->width = width;
		this->height = height;
		RenderTextureFormat rtFormat;
		rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
		rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		lastRenderTarget = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, rtFormat, RenderTextureDimension_Tex2D, 0, 0)
			);
		rtFormat.colorFormat = DXGI_FORMAT_R32_FLOAT;
		lastDepthTexture = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, rtFormat, RenderTextureDimension_Tex2D, 0, 0)
			);
		rtFormat.colorFormat = DXGI_FORMAT_R16G16_SNORM;
		lastMotionVectorTexture = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, rtFormat, RenderTextureDimension_Tex2D, 0, 0)
			);
		Graphics::ResourceStateTransform(cmdList, lastRenderTarget->GetWriteState(), lastRenderTarget->GetReadState(), lastRenderTarget->GetColorResource());
		Graphics::ResourceStateTransform(cmdList, lastDepthTexture->GetWriteState(), lastDepthTexture->GetReadState(), lastDepthTexture->GetColorResource());
		Graphics::ResourceStateTransform(cmdList, lastMotionVectorTexture->GetWriteState(), lastMotionVectorTexture->GetReadState(), lastMotionVectorTexture->GetColorResource());
	}
public:
	TAACameraData(UINT width, UINT height, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
	{
		Update(width, height, device, cmdList);
	}

	bool UpdateFrame(UINT width, UINT height, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
	{
		if (width != this->width || height != this->height)
		{
			Update(width, height, device, cmdList);
			return true;
		}
		return false;
	}
};

class TemporalAA
{
public:
	PrepareComponent* prePareComp;
	ID3D12Device* device;
	PSOContainer* toRTContainer;
	UINT TAAConstBuffer_Index;
	Shader* taaShader;
	TemporalAA()
	{
		TAAConstBuffer_Index = ShaderID::PropertyToID("TAAConstBuffer");
		taaShader = ShaderCompiler::GetShader("TemporalAA");
	}
	void Run(
		RenderTexture* inputColorBuffer,
		RenderTexture* inputDepthBuffer,
		RenderTexture* motionVector,
		RenderTexture* renderTargetTex,
		ThreadCommand* tCmd,
		FrameResource* res,
		Camera* cam,
		UINT width, UINT height)
	{
		auto commandList = tCmd->GetCmdList();
		CameraTransformData* camTransData = (CameraTransformData*)cam->GetResource(prePareComp, [&]()->CameraTransformData*
		{
			return new CameraTransformData;
		});
		TAACameraData* tempCamData = (TAACameraData*)cam->GetResource(this, [&]()->TAACameraData*
		{
			return new TAACameraData(width, height, device, commandList);
		});
		TAAFrameData* tempFrameData = (TAAFrameData*)res->GetPerCameraResource(this, cam, [&]()->TAAFrameData*
		{
			return new TAAFrameData(device);
		});
		RenderTextureStateBarrier barrier(inputColorBuffer, commandList, ResourceReadWriteState::Read);
		RenderTextureStateBarrier barrier1(inputDepthBuffer, commandList, ResourceReadWriteState::Read);
		RenderTextureStateBarrier barrier2(motionVector, commandList, ResourceReadWriteState::Read);
		if (tempCamData->UpdateFrame(width, height, device, commandList))
		{
			//Refresh & skip
			Graphics::ResourceStateTransform(commandList, renderTargetTex->GetWriteState(), D3D12_RESOURCE_STATE_COPY_DEST, renderTargetTex->GetColorResource());
			Graphics::CopyTexture(commandList, inputColorBuffer, 0, 0, renderTargetTex, 0, 0);
			Graphics::ResourceStateTransform(commandList,D3D12_RESOURCE_STATE_COPY_DEST, renderTargetTex->GetWriteState(), renderTargetTex->GetColorResource());
		}
		else
		{
			TAAConstBuffer constBufferData;
			inputColorBuffer->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 0, device);
			inputDepthBuffer->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 1, device);
			motionVector->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 2, device);
			tempCamData->lastDepthTexture->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 3, device);
			tempCamData->lastMotionVectorTexture->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 4, device);
			tempCamData->lastRenderTarget->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 5, device);
			constBufferData._InvNonJitterVP = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(camTransData->nonJitteredVPMatrix), camTransData->nonJitteredVPMatrix);
			constBufferData._InvLastVp = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(camTransData->lastNonJitterVP), camTransData->lastNonJitterVP);
			constBufferData._FinalBlendParameters = { 0.95f,0.85f, 6000.0f, 0 };		//Stationary Move, Const, 0
			constBufferData._CameraDepthTexture_TexelSize = { 1.0f / width, 1.0f / height, (float)width, (float)height };
			constBufferData._ScreenParams = { (float)width, (float)height, 1.0f / width + 1, 1.0f / height + 1 };
			constBufferData._ZBufferParams = prePareComp->_ZBufferParams;
			constBufferData._TemporalClipBounding = { 3.0f, 1.25f, 3000.0f };
			constBufferData._Sharpness = 0.02f;
			constBufferData._Jitter = camTransData->jitter;
			constBufferData._LastJitter = camTransData->lastFrameJitter;
			tempFrameData->taaBuffer.CopyData(0, &constBufferData);

			taaShader->BindRootSignature(commandList, &tempFrameData->srvHeap);
			taaShader->SetResource(commandList, TAAConstBuffer_Index, &tempFrameData->taaBuffer, 0);
			taaShader->SetResource(commandList, ShaderID::GetMainTex(), &tempFrameData->srvHeap, 0);

			Graphics::Blit(
				commandList,
				device,
				&renderTargetTex->GetColorDescriptor(0), 1,
				nullptr,
				toRTContainer, 0,
				width, height,
				taaShader, 0
			);

		}
		
		Graphics::ResourceStateTransform(commandList, tempCamData->lastRenderTarget->GetReadState(), D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastRenderTarget->GetColorResource());
		Graphics::ResourceStateTransform(commandList, tempCamData->lastMotionVectorTexture->GetReadState(), D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastMotionVectorTexture->GetColorResource());
		Graphics::ResourceStateTransform(commandList, tempCamData->lastDepthTexture->GetReadState(), D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastDepthTexture->GetColorResource());
		Graphics::ResourceStateTransform(commandList, renderTargetTex->GetWriteState(), D3D12_RESOURCE_STATE_COPY_SOURCE, renderTargetTex->GetColorResource());
		Graphics::ResourceStateTransform(commandList, inputDepthBuffer->GetReadState(), D3D12_RESOURCE_STATE_COPY_SOURCE, inputDepthBuffer->GetColorResource());
		Graphics::CopyTexture(commandList, renderTargetTex, 0, 0, tempCamData->lastRenderTarget.get(), 0, 0);
		Graphics::CopyTexture(commandList, motionVector, 0, 0, tempCamData->lastMotionVectorTexture.get(), 0, 0);
		Graphics::CopyTexture(commandList, inputDepthBuffer, 0, 0, tempCamData->lastDepthTexture.get(), 0, 0);
		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastRenderTarget->GetReadState(), tempCamData->lastRenderTarget->GetColorResource());
		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastMotionVectorTexture->GetReadState(), tempCamData->lastMotionVectorTexture->GetColorResource());
		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_DEST, tempCamData->lastDepthTexture->GetReadState(), tempCamData->lastDepthTexture->GetColorResource());
		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, renderTargetTex->GetWriteState(), renderTargetTex->GetColorResource());
		Graphics::ResourceStateTransform(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, inputDepthBuffer->GetReadState(), inputDepthBuffer->GetColorResource());
	}
};


Storage<TemporalAA, 1> taaStorage;
std::vector<TemporalResourceCommand>& TemporalAntiAlias::SendRenderTextureRequire(EventData& evt)
{
	for (uint i = 0; i < tempRT.size(); ++i)
	{
		auto& a = tempRT[i].descriptor.rtDesc;
		a.width = evt.width;
		a.height = evt.height;
	}
	return tempRT;
}

void TemporalAntiAlias::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	RenderTexture* sourceTex;
	RenderTexture* destTex;
	RenderTexture* depthTex;
	RenderTexture* motionVector;
	sourceTex = (RenderTexture*)allTempResource[0];
	destTex = (RenderTexture*)allTempResource[1];
	depthTex = (RenderTexture*)allTempResource[2];
	motionVector = (RenderTexture*)allTempResource[3];
	FrameResource* res = data.resource;
	Camera* cam = data.camera;
	uint width = data.width;
	uint height = data.height;
	ScheduleJob([=]()->void
	{
		TemporalAA* ptr = (TemporalAA*)&taaStorage;
		commandList->ResetCommand();
		ptr->Run(
			sourceTex,
			depthTex,
			motionVector,
			destTex,
			commandList,
			res,
			cam,
			width,
			height);
		commandList->CloseCommand();
	});
}

void TemporalAntiAlias::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	tempRT.resize(4);
	tempRT[0].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_PostProcessBlitTarget");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_CameraDepthTexture");
	tempRT[3].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[3].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	auto& desc = tempRT[1].descriptor;
	desc.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	desc.rtDesc.depthSlice = 1;
	desc.rtDesc.width = 0;
	desc.rtDesc.height = 0;
	desc.rtDesc.type = RenderTextureDimension_Tex2D;
	desc.rtDesc.state = RenderTextureState::Render_Target;
	renderTextureContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &desc.rtDesc.rtFormat.colorFormat)
		);
	TemporalAA* ptr = (TemporalAA*)&taaStorage;
	ptr->prePareComp = RenderPipeline::GetComponent<PrepareComponent>();
	ptr->device = device;
	ptr->toRTContainer = renderTextureContainer.get();
	new (ptr)TemporalAA();
}

void TemporalAntiAlias::Dispose()
{
	TemporalAA* ptr = (TemporalAA*)&taaStorage;
	ptr->~TemporalAA();
	renderTextureContainer = nullptr;
}