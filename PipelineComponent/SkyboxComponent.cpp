#include "SkyboxComponent.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
#include "../RenderComponent/Texture.h"
//#include "BaseColorComponent.h"
Skybox* defaultSkybox;
std::unique_ptr<PSOContainer> psoContainer;
using namespace DirectX;
struct SkyboxBuffer
{
	XMFLOAT4X4 invvp;
};

class SkyboxPerFrameData : public IPipelineResource
{
public:
	UploadBuffer posBuffer;
	SkyboxPerFrameData(ID3D12Device* device) : 
		posBuffer(device,
			1, true,
			sizeof(SkyboxBuffer))
	{
	}
};

XMMATRIX CalculateViewMatrix(Camera* cam)
{
	XMVECTOR R = cam->GetRight();
	XMVECTOR U = cam->GetUp();
	XMVECTOR L = cam->GetLook();
	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L);

	XMFLOAT3* mRight = (XMFLOAT3*)&R;
	XMFLOAT3* mUp = (XMFLOAT3*)&U;
	XMFLOAT3* mLook = (XMFLOAT3*)&L;
	XMFLOAT4X4 mView;
	mView(0, 0) = mRight->x;
	mView(1, 0) = mRight->y;
	mView(2, 0) = mRight->z;
	mView(3, 0) = 0;

	mView(0, 1) = mUp->x;
	mView(1, 1) = mUp->y;
	mView(2, 1) = mUp->z;
	mView(3, 1) = 0;

	mView(0, 2) = mLook->x;
	mView(1, 2) = mLook->y;
	mView(2, 2) = mLook->z;
	mView(3, 2) = 0;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
	return *(XMMATRIX*)&mView;
}

class SkyboxRunnable
{
public:
	RenderTexture* gbufferTex;
	RenderTexture* mvTex;
	RenderTexture* depthTex;
	SkyboxComponent* selfPtr;
	ThreadCommand* commandList;
	FrameResource* resource;
	ID3D12Device* device;
	Camera* cam;
	void operator()()
	{
		commandList->ResetCommand();
		if (psoContainer == nullptr)
		{
			DXGI_FORMAT rtFormats[2];
			rtFormats[0] = gbufferTex->GetColorFormat();
			rtFormats[1] = mvTex->GetColorFormat();
			
			psoContainer = std::unique_ptr<PSOContainer>(
				new PSOContainer(depthTex->GetColorFormat(), 2, rtFormats)
				);
		}
		SkyboxPerFrameData* frameData = (SkyboxPerFrameData*)resource->GetPerCameraResource(selfPtr, cam, [&]()->SkyboxPerFrameData*
		{
			return new SkyboxPerFrameData(device);
		});
		XMMATRIX view = CalculateViewMatrix(cam);
		XMMATRIX viewProj = XMMatrixMultiply(view, cam->GetProj());
		SkyboxBuffer bf;
		memcpy(&bf.invvp, &XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj), sizeof(XMFLOAT4X4));
		frameData->posBuffer.CopyData(0, &bf);
		ID3D12GraphicsCommandList* cmdList = commandList->GetCmdList();
		gbufferTex->SetViewport(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[2];
		rtHandles[0] = gbufferTex->GetColorDescriptor(0);
		rtHandles[1] = mvTex->GetColorDescriptor(0);
		D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = depthTex->GetColorDescriptor(0);
		auto format = depthTex->GetColorFormat();
		cmdList->OMSetRenderTargets(
			2,
			rtHandles,
			false,
			&depthHandle
		);
		ConstBufferElement skyboxData;
		skyboxData.buffer = &frameData->posBuffer;
		skyboxData.element = 0;
		defaultSkybox->Draw(
			0,
			commandList->GetCmdList(),
			device,
			&skyboxData,
			resource,
			psoContainer.get(), 0
		);
		commandList->CloseCommand();
	}
};

std::vector<TemporalResourceCommand>& SkyboxComponent::SendRenderTextureRequire(EventData& evt)
{
	tempRT[0].descriptor.rtDesc.width = evt.width;
	tempRT[0].descriptor.rtDesc.height = evt.height;
	return tempRT;
}
void SkyboxComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<SkyboxRunnable>(
		{
			 (RenderTexture*)allTempResource[0],
			  (RenderTexture*)allTempResource[1],
			  (RenderTexture*)allTempResource[2],
			 this,
			 commandList,
			 data.resource,
			 data.device,
			 data.camera
		});
}


void SkyboxComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	//SetGPUDepending<BaseColorComponent>();
	tempRT.resize(3);

	TemporalResourceCommand& emissionBuffer = tempRT[0];
	emissionBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	emissionBuffer.descriptor.rtDesc.depthSlice = 1;
	emissionBuffer.descriptor.rtDesc.type = RenderTextureDimension::RenderTextureDimension_Tex2D;
	emissionBuffer.descriptor.rtDesc.state = RenderTextureState::Render_Target;
	tempRT[1].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_CameraDepthTexture");

	skyboxTex = new Texture(
		device,
		"grasscube1024",
		L"Resource/Sky.vtex",
		TextureType::Cubemap
	);
	defaultSkybox = new Skybox(
		skyboxTex,
		device
	);
}
void SkyboxComponent::Dispose()
{
	psoContainer = nullptr;
	delete defaultSkybox;
}