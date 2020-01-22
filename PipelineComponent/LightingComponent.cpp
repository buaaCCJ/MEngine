#include "LightingComponent.h"
#include "PrepareComponent.h"
#include "../RenderComponent/Light.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
using namespace DirectX;

#define XRES 32
#define YRES 16
#define ZRES 64
#define VOXELZ 64
#define MAXLIGHTPERCLUSTER 128
#define FROXELRATE 1.35
#define CLUSTERRATE 1.5

std::vector<LightCommand> lights;
ComputeShader* lightCullingShader;


UINT _LightIndexBuffer;
UINT _AllLight;
UINT LightCullCBufferID;


struct LightCullCBuffer
{
	XMFLOAT4X4 _InvVP;
	XMFLOAT4 _CameraNearPos;
	XMFLOAT4 _CameraFarPos;
	XMFLOAT4 _ZBufferParams;
	//Align
	XMFLOAT3 _CameraForward;
	UINT _LightCount;
	//Align
	XMFLOAT3 _SunColor;
	UINT _SunEnabled;
	XMFLOAT3 _SunDir;
	UINT _SunShadowEnabled;
	XMUINT4 _ShadowmapIndices;
	XMFLOAT4 _CascadeDistance;
	XMFLOAT4X4 _ShadowMatrix[4];
};
void LightingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	lights.reserve(50);
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	lightCullingShader = ShaderCompiler::GetComputeShader("LightCull");
	RenderTextureFormat xyPlaneFormat;
	xyPlaneFormat.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	xyPlaneFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	xyPlaneTexture = std::unique_ptr<RenderTexture>(
		new RenderTexture(
			device, XRES, YRES, xyPlaneFormat, RenderTextureDimension_Tex3D, 4, 0, RenderTextureState::Unordered_Access
		));
	zPlaneTexture = std::unique_ptr<RenderTexture>(
		new RenderTexture(device, ZRES, 2, xyPlaneFormat, RenderTextureDimension_Tex2D, 1, 0, RenderTextureState::Unordered_Access
		));
	StructuredBufferElement ele;
	ele.elementCount = XRES * YRES * ZRES * (MAXLIGHTPERCLUSTER + 1);
	ele.stride = sizeof(UINT);
	lightIndexBuffer = std::unique_ptr<StructuredBuffer>(
		new StructuredBuffer(
			device, &ele, 1
		));
	cullingDescHeap = std::make_unique<DescriptorHeap>();
	cullingDescHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true);
	xyPlaneTexture->BindUAVToHeap(cullingDescHeap.get(), 0, device, 0);
	zPlaneTexture->BindUAVToHeap(cullingDescHeap.get(), 1, device, 0);
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	_AllLight = ShaderID::PropertyToID("_AllLight");
	LightCullCBufferID = ShaderID::PropertyToID("LightCullCBuffer");
}
void LightingComponent::Dispose()
{
	xyPlaneTexture = nullptr;
	zPlaneTexture = nullptr;
	lightIndexBuffer = nullptr;
	cullingDescHeap = nullptr;
}
std::vector<TemporalResourceCommand>& LightingComponent::SendRenderTextureRequire(EventData& evt)
{
	return tempResources;
}

LightFrameData::LightFrameData(ID3D12Device* device)
{
	lightsInFrustum.Create(device, 50, false, sizeof(LightCommand));
	lightCBuffer.Create(device, 1, true, sizeof(LightCullCBuffer));
}
struct LightingRunnable
{
	ThreadCommand* tcmd;
	ID3D12Device* device;
	Camera* cam;
	FrameResource* res;
	LightingComponent* selfPtr;
	PrepareComponent* prepareComp;
	float clusterLightFarPlane;
	void operator()()
	{
		tcmd->ResetCommand();
		auto commandList = tcmd->GetCmdList();
		XMVECTOR vec[6];
		memcpy(vec, prepareComp->frustumPlanes, sizeof(XMVECTOR) * 6);
		XMVECTOR camForward = cam->GetLook();
		vec[0] = MathLib::GetPlane(std::move(camForward), std::move(cam->GetPosition() + min(cam->GetFarZ(), clusterLightFarPlane) * camForward));
		Light::GetLightingList(lights,
			vec,
			std::move(prepareComp->frustumMinPos),
			std::move(prepareComp->frustumMaxPos));
		LightFrameData* lightData = (LightFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->LightFrameData*
		{
			return new LightFrameData(device);
		});
		if (lightData->lightsInFrustum.GetElementCount() < lights.size())
		{
			UINT maxSize = max(lights.size(), (UINT)(lightData->lightsInFrustum.GetElementCount() * 1.5));
			lightData->lightsInFrustum.Create(device, maxSize, false, sizeof(LightCommand));
		}
		LightCullCBuffer& cb = *(LightCullCBuffer*)lightData->lightCBuffer.GetMappedDataPtr(0);
		XMVECTOR farPos = cam->GetPosition() + clusterLightFarPlane * cam->GetLook();
		memcpy(&cb._CameraFarPos, &farPos, sizeof(XMFLOAT3));
		cb._CameraFarPos.w = clusterLightFarPlane;
		XMVECTOR nearPos = cam->GetPosition() + cam->GetNearZ() * cam->GetLook();
		memcpy(&cb._CameraNearPos, &nearPos, sizeof(XMFLOAT3));
		cb._CameraNearPos.w = cam->GetNearZ();
		cb._InvVP = prepareComp->passConstants.InvViewProj;
		cb._ZBufferParams = prepareComp->_ZBufferParams;
		cb._CameraForward = cam->GetLook3f();
		cb._LightCount = lights.size();
		//Test Sun Light
		cb._SunColor = { 1, 1, 1 };
		cb._SunEnabled = 1;
		cb._SunDir = { 30, 300, 0 };
		cb._SunShadowEnabled = 0;
		cb._ShadowmapIndices = {0,0,0,0};
		//
		lightData->lightsInFrustum.CopyDatas(0, lights.size(), lights.data());
		lightCullingShader->BindRootSignature(commandList, selfPtr->cullingDescHeap.get());
		lightCullingShader->SetResource(commandList, _LightIndexBuffer, selfPtr->lightIndexBuffer.get(), 0);
		lightCullingShader->SetResource(commandList, _AllLight, &lightData->lightsInFrustum, 0);
		lightCullingShader->SetResource(commandList, LightCullCBufferID, &lightData->lightCBuffer, 0);
		lightCullingShader->SetResource(commandList, ShaderID::GetMainTex(), selfPtr->cullingDescHeap.get(), 0);
		lightCullingShader->Dispatch(commandList, 0, 1, 1, 1);//Set XY Plane
		lightCullingShader->Dispatch(commandList, 1, 1, 1, 1);//Set Z Plane
		lightCullingShader->Dispatch(commandList, 2, 1, 1, ZRES);
		tcmd->CloseCommand();
	}
};
void LightingComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<LightingRunnable>({
		commandList,
		data.device,
		data.camera,
		data.resource,
		this,
		prepareComp,
		128
		});
}