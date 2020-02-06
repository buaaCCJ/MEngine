#include "CSMComponent.h"
#include "PrepareComponent.h"
#include "LightingComponent.h"
#include "../RenderComponent/Light.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../LogicComponent/Transform.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../RenderComponent/GPU Driven/GRP_Renderer.h"
#include "../LogicComponent/World.h"
namespace CSM
{
	PrepareComponent* prepareComp;
	const float zDepth = 500;
}
using namespace CSM;
using namespace Math;
struct ShadowmapData
{
	Matrix4 vpMatrix;
	Vector4 position;
	float size;
};


void GetCascadeShadowmapMatrices(
	const Vector3& sunRight,
	const Vector3& sunUp,
	const Vector3& sunForward,
	const Vector3& cameraRight,
	const Vector3& cameraUp,
	const Vector3& cameraForward,
	const Vector3& cameraPosition,
	float fov,
	float aspect,
	float* distances,
	uint distanceCount,
	ShadowmapData* results)
{
	Matrix4 cameraLocalToWorld = GetTransformMatrix(
		cameraRight,
		cameraUp,
		cameraForward,
		cameraPosition);
	Matrix4 sunLocalToWorld = transpose(GetTransformMatrix(
		sunRight,
		sunUp,
		sunForward,
		Vector3(0, 0, 0)
	));

	Vector3 corners[8];
	Vector3* lastCorners = corners;
	Vector3* nextCorners = corners + 4;
	MathLib::GetCameraNearPlanePoints(
		std::move((Matrix4&)cameraLocalToWorld),
		fov,
		aspect, distances[0], nextCorners);
	for (uint i = 0; i < distanceCount - 1; ++i)
	{
		{
			Vector3* swaper = lastCorners;
			lastCorners = nextCorners;
			nextCorners = swaper;
		}
		MathLib::GetCameraNearPlanePoints(
			std::move((Matrix4&)cameraLocalToWorld),
			fov,
			aspect, distances[i + 1], nextCorners);
		float farDist = distance(nextCorners[0], nextCorners[3]);
		float crossDist = distance(lastCorners[0], nextCorners[3]);
		float maxDist = max(farDist, crossDist) * 0.5f;
		Matrix4 projMatrix = XMMatrixOrthographicLH(maxDist, maxDist, -zDepth, zDepth);
		Matrix4 sunWorldToLocal = inverse(sunLocalToWorld);
		Vector4 minBoundingPoint, maxBoundingPoint;
		{
			Vector4& c = (Vector4&)corners[0];
			c.SetW(1);
			minBoundingPoint = mul(sunWorldToLocal, c);
			maxBoundingPoint = minBoundingPoint;
		}
		for (uint j = 1; j < 8; ++j)
		{
			Vector4& c = (Vector4&)corners[j];
			c.SetW(1);
			Vector4 pointLocalPos = mul(sunWorldToLocal, c);
			minBoundingPoint = min(pointLocalPos, minBoundingPoint);
			maxBoundingPoint = max(pointLocalPos, maxBoundingPoint);
		}
		minBoundingPoint = mul(sunLocalToWorld, minBoundingPoint);
		maxBoundingPoint = mul(sunLocalToWorld, maxBoundingPoint);
		Vector3 position = minBoundingPoint + maxBoundingPoint;
		//TODO
		//Chess Board Transformation
		Matrix4 viewMat = GetInverseTransformMatrix(
			sunRight,
			sunUp,
			sunForward,
			position);
		results[i] =
		{
			mul(viewMat, projMatrix),
			position,
			maxDist
		};
	}
}
class CSMFrameData : public IPipelineResource
{
public:
	UploadBuffer cullBuffer;
	CSMFrameData(ID3D12Device* device) : cullBuffer(device, 1, true, sizeof(GRP_Renderer::CullData))
	{
	}
};
class CSMRunnable
{
public:

	Camera* cam;
	FrameResource* res;
	ThreadCommand* tCmd;
	CSMComponent* selfPtr;
	ID3D12Device* device;
	World* world;
	void operator()()
	{
		tCmd->ResetCommand();
		auto commandList = tCmd->GetCmdList();
		DirectionalLight* dLight = DirectionalLight::GetInstance();
		if (dLight)
		{
			float dist[DirectionalLight::CascadeLevel + 1];
			ShadowmapData vpMatrices[DirectionalLight::CascadeLevel];
			dist[0] = cam->GetNearZ();
			memcpy(dist + 1, &dLight->shadowDistance, sizeof(float) * DirectionalLight::CascadeLevel);
			Vector3 sunRight = dLight->transform->GetRight();
			Vector3 sunUp = dLight->transform->GetUp();
			Vector3 sunForward = dLight->transform->GetForward();
			GetCascadeShadowmapMatrices(
				sunRight,
				sunUp,
				sunForward,
				cam->GetRight(),
				cam->GetUp(),
				cam->GetLook(),
				cam->GetPosition(),
				cam->GetFovY(),
				cam->GetAspect(),
				dist,
				DirectionalLight::CascadeLevel + 1,
				vpMatrices);
			CSMFrameData* frameData = (CSMFrameData*)res->GetPerCameraResource(selfPtr, cam, [=]()->CSMFrameData*
			{
				return new CSMFrameData(device);
			});
			Vector4 frustumPlanes[6];
			Vector3 frustumPoints[8];
			float4 frustumPInFloat[6];
			Vector3 frustumMinPoint, frustumMaxPoint;
			for (uint i = 0; i < DirectionalLight::CascadeLevel; ++i)
			{
				auto& m = vpMatrices[i];
				MathLib::GetOrthoCamFrustumPlanes(sunRight, sunUp, sunForward, m.position, m.size, m.size, -zDepth, zDepth, frustumPlanes);
				MathLib::GetOrthoCamFrustumPoints(sunRight, sunUp, sunForward, m.position, m.size, m.size, -zDepth, zDepth, frustumPoints);
				for (uint i = 0; i < 6; ++i)
				{
					frustumPInFloat[i] = frustumPlanes[i];
				}
				frustumMinPoint = frustumPoints[0];
				frustumMaxPoint = frustumMinPoint;
				for (uint i = 1; i < 8; ++i)
				{
					frustumMinPoint = min(frustumMinPoint, frustumPoints[i]);
					frustumMaxPoint = max(frustumMaxPoint, frustumPoints[i]);
				}
				world->grpRenderer->Culling(
					commandList,
					device,
					res,
					{ &frameData->cullBuffer, 0 },
					frustumPInFloat,
					frustumMinPoint,
					frustumMaxPoint);
				commandList->OMSetRenderTargets(
					0, nullptr, false, &dLight->GetShadowmap(i)->GetColorDescriptor(0));
				/*world->grpRenderer->DrawCommand(
					commandList,
					device,
					2, 
				)*/
				//TODO
				//Write Shader
			}
		}
		tCmd->CloseCommand();
	}
};

void CSMComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	SetCPUDepending<PrepareComponent>();
}
void CSMComponent::Dispose()
{

}
std::vector<TemporalResourceCommand>& CSMComponent::SendRenderTextureRequire(EventData& evt)
{
	return tempResources;
}
void CSMComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob< CSMRunnable>(
		{
			data.camera,
			data.resource,
			commandList,
			this,
			data.device,
			data.world
		});
}