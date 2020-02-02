#include "LightingComponent.h"
#include "PrepareComponent.h"
#include "../RenderComponent/Light.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "RenderPipeline.h"
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


uint _LightIndexBuffer;
uint _AllLight;
uint LightCullCBufferID;


struct LightCullCBuffer
{
	float4x4 _InvVP;
	float4 _CameraNearPos;
	float4 _CameraFarPos;
	float4 _ZBufferParams;
	//Align
	float3 _CameraForward;
	uint _LightCount;
	//Align
	float3 _SunColor;
	uint _SunEnabled;
	float3 _SunDir;
	uint _SunShadowEnabled;
	uint4 _ShadowmapIndices;
	float4 _CascadeDistance;
	float4x4 _ShadowMatrix[4];
	uint _ReflectionProbeCount;
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
	ele.stride = sizeof(uint);
	lightIndexBuffer = std::unique_ptr<StructuredBuffer>(
		new StructuredBuffer(
			device, &ele, 1
		));
	cullingDescHeap = std::unique_ptr<DescriptorHeap>(new DescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true));
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

LightFrameData::LightFrameData(ID3D12Device* device) : 
	lightsInFrustum(device, 50, false, sizeof(LightCommand)),
	lightCBuffer(device, 1, true, sizeof(LightCullCBuffer))
{
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
			uint maxSize = max(lights.size(), (uint)(lightData->lightsInFrustum.GetElementCount() * 1.5));
			lightData->lightsInFrustum.Create(device, maxSize, false, sizeof(LightCommand));
		}
		LightCullCBuffer& cb = *(LightCullCBuffer*)lightData->lightCBuffer.GetMappedDataPtr(0);
		XMVECTOR farPos = cam->GetPosition() + clusterLightFarPlane * cam->GetLook();
		memcpy(&cb._CameraFarPos, &farPos, sizeof(float3));
		cb._CameraFarPos.w = clusterLightFarPlane;
		XMVECTOR nearPos = cam->GetPosition() + cam->GetNearZ() * cam->GetLook();
		memcpy(&cb._CameraNearPos, &nearPos, sizeof(float3));
		cb._CameraNearPos.w = cam->GetNearZ();
		cb._InvVP = prepareComp->passConstants.InvViewProj;
		cb._ZBufferParams = prepareComp->_ZBufferParams;
		cb._CameraForward = cam->GetLook3f();
		cb._LightCount = lights.size();
		//Test Sun Light
		cb._SunColor = { 3 / 2.0, 2.8 / 2.0, 2.65 / 2.0 };
		cb._SunEnabled = 1;
		cb._SunDir = { -0.75, -0.5, 0.4330126 };
		cb._SunShadowEnabled = 0;
		cb._ShadowmapIndices = { 0,0,0,0 };

		if (!lights.empty())
		{
			lightData->lightsInFrustum.CopyDatas(0, lights.size(), lights.data());
		}
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