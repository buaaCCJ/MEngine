#include "PrepareComponent.h"
#include "../Common/Camera.h"
#include "../RenderComponent/UploadBuffer.h"
#include "CameraData/CameraTransformData.h"
#include "../Singleton/MathLib.h"
using namespace Math;
UINT sampleIndex = 0;
const UINT k_SampleCount = 8;
double GetHalton(int index, int radix)
{
	double result = 0;
	double fraction = 1.0 / (double)radix;

	while (index > 0)
	{
		result += (double)(index % radix) * fraction;

		index /= radix;
		fraction /= (double)radix;
	}

	return result;
}
XMVECTOR GenerateRandomOffset()
{
	XMVECTOR offset = {
		GetHalton((sampleIndex & 1023) + 1, 2) - 0.5,
		GetHalton((sampleIndex & 1023) + 1, 3) - 0.5,
		0, 0
	};

	if (++sampleIndex >= k_SampleCount)
		sampleIndex = 0;

	return offset;
}

void GetJitteredPerspectiveProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	UINT pixelWidth, UINT pixelHeight,
	XMFLOAT2 offset, XMMATRIX* projectionMatrix)
{
	double nearPlane = nearClipPlane;
	double vertical = tan(0.5 * Deg2Rad * fieldOfView) * nearPlane;
	double horizontal = vertical * aspect;

	offset.x *= horizontal / (0.5f * pixelWidth);
	offset.y *= vertical / (0.5f * pixelHeight);

	XMFLOAT4X4* projPt = (XMFLOAT4X4*)projectionMatrix;
	projPt->m[2][0] -= offset.x / horizontal;
	projPt->m[2][1] += offset.y / vertical;		//TODD: dont know difference between ogl & dx
}
void GetJitteredProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	double jitterOffset,
	UINT pixelWidth, UINT pixelHeight,
	XMMATRIX& projectionMatrix,
	XMVECTOR& jitter)
{
	jitter = GenerateRandomOffset();
	jitter *= jitterOffset;
	XMFLOAT2* jitterPtr = (XMFLOAT2*)&jitter;
	GetJitteredPerspectiveProjectionMatrix(
		nearClipPlane, farClipPlane,
		fieldOfView,
		aspect,
		pixelWidth, pixelHeight,
		*jitterPtr,
		&projectionMatrix
	);
	*jitterPtr = { jitterPtr->x / pixelWidth, jitterPtr->y / pixelHeight };
}
void ConfigureJitteredProjectionMatrix(Camera* camera, UINT height, UINT width, double jitterOffset, CameraTransformData* data)
{
	data->p = camera->GetProj();
	data->nonJitteredProjMatrix = data->p;
	data->lastFrameJitter = data->jitter;
	XMVECTOR jitterVec = XMLoadFloat2(&data->jitter);
	GetJitteredProjectionMatrix(
		camera->GetNearZ(),
		camera->GetFarZ(),
		camera->GetFovY(),
		camera->GetAspect(),
		jitterOffset,
		width, height,
		data->p,
		jitterVec
	);
	memcpy(&data->jitter, &jitterVec, sizeof(XMFLOAT2));
	camera->SetProj(data->p);
	data->lastNonJitterVP = data->nonJitteredVPMatrix;
	data->nonJitteredVPMatrix = XMMatrixMultiply(camera->GetView(), data->nonJitteredProjMatrix);
	data->vp = XMMatrixMultiply(camera->GetView(), data->p);
}

struct PrepareRunnable
{
	PrepareComponent* ths;
	FrameResource* resource;
	Camera* camera;
	UINT width, height;
	void operator()()
	{
		CameraTransformData* transData = (CameraTransformData*)camera->GetResource(ths, []()->CameraTransformData*
		{
			return new CameraTransformData;
		});
		camera->UpdateProjectionMatrix();
		camera->UpdateViewMatrix();
		ConfigureJitteredProjectionMatrix(
			camera,
			width, height,
			1, transData
		);
		ths->_ZBufferParams.x = (float)(1.0 - (camera->GetFarZ() / camera->GetNearZ()));
		ths->_ZBufferParams.y = (float)(camera->GetFarZ() / camera->GetNearZ());
		ths->_ZBufferParams.z = ths->_ZBufferParams.x / camera->GetFarZ();
		ths->_ZBufferParams.w = ths->_ZBufferParams.y / camera->GetFarZ();
		camera->UploadCameraBuffer(ths->passConstants);
		//Calculate Jitter Matrix
		XMMATRIX nonJitterVP = transData->nonJitteredVPMatrix;
		ths->passConstants.nonJitterVP = *(XMFLOAT4X4*)&nonJitterVP;
		ths->passConstants.nonJitterInverseVP = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(nonJitterVP), nonJitterVP);
		memcpy(&ths->passConstants.lastVP, &transData->lastNonJitterVP, sizeof(XMFLOAT4X4));
		ths->passConstants.lastInverseVP = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(transData->lastNonJitterVP), transData->lastNonJitterVP);
		ConstBufferElement ele = resource->cameraCBs[camera->GetInstanceID()];
		ele.buffer->CopyData(ele.element, &ths->passConstants);
		//Calculate Frustum Planes
		Matrix4 localToWorldMatrix;
		localToWorldMatrix[0] = camera->GetRight();
		localToWorldMatrix[1] = camera->GetUp();
		localToWorldMatrix[2] = camera->GetLook();
		XMFLOAT3 position = camera->GetPosition3f();
		localToWorldMatrix[3] = { position.x, position.y, position.z, 1 };
		MathLib::GetPerspFrustumPlanes(std::move(localToWorldMatrix), camera->GetFovY(), camera->GetAspect(), camera->GetNearZ(), camera->GetFarZ(), ths->frustumPlanes);
		//Calculate Frustum Bounding
		MathLib::GetFrustumBoundingBox(
			std::move(localToWorldMatrix),
			camera->GetNearWindowHeight(),
			camera->GetFarWindowHeight(),
			camera->GetAspect(),
			camera->GetNearZ(),
			camera->GetFarZ(),
			&ths->frustumMinPos,
			&ths->frustumMaxPos
		);
	}
};

void PrepareComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		data.resource,
		data.camera,
		data.width, data.height
	};
	ScheduleJob(runnable);
}