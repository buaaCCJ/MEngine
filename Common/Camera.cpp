//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Camera.h"
#include "../RenderComponent/CBufferPool.h"
#include "../RenderComponent/UploadBuffer.h"
using namespace DirectX;
using namespace Math;

Camera::Camera(ID3D12Device* device, CameraRenderPath renderType) : MObject(), renderType(renderType)
{
	perCameraResource.reserve(20);
	SetLens(0.25f*MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
	for (auto ite = FrameResource::mFrameResources.begin(); ite != FrameResource::mFrameResources.end(); ++ite)
	{
		if (*ite != nullptr)
			(*ite)->OnLoadCamera(this, device);
	}
}

Camera::~Camera()
{
	for (auto ite = FrameResource::mFrameResources.begin(); ite != FrameResource::mFrameResources.end(); ++ite)
	{
		if (*ite != nullptr)
			(*ite)->OnUnloadCamera(this);
	}
	for (auto ite = perCameraResource.begin(); ite != perCameraResource.end(); ++ite)
	{
		if (ite->second != nullptr)
		{
			delete ite->second;
		}
	}
}

void Camera::UploadCameraBuffer(PassConstants& mMainPassCB)
{
	UpdateViewMatrix();
	Matrix4 view = GetView();
	Matrix4 proj = GetProj();
	Matrix4 viewProj = XMMatrixMultiply(view, proj);
	Matrix4 invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	Matrix4 invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	Matrix4 invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, (view));
	XMStoreFloat4x4(&mMainPassCB.InvView, (invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, (proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, (invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, (viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, (invViewProj));
	mMainPassCB.NearZ = GetNearZ();
	mMainPassCB.worldSpaceCameraPos = GetPosition();
	mMainPassCB.FarZ = GetFarZ();
	//auto& currPassCB = res->cameraCBs[GetInstanceID()];
	//currPassCB.buffer->CopyData(currPassCB.element, &mMainPassCB);
}
void Camera::SetPosition(double x, double y, double z)
{
	mPosition = XMFLOAT3(x, y, z);
	mViewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	mPosition = v;
	mViewDirty = true;
}

void Camera::SetLens(double fovY, double aspect, double zn, double zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;
	mNearWindowHeight = 2.0 * (double)mNearZ * tan(0.5 * mFovY);
	mFarWindowHeight = 2.0 * (double)mFarZ * tan(0.5 * mFovY);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);

	mViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	mViewDirty = true;
}


void Camera::SetProj(const DirectX::XMFLOAT4X4& data)
{
	memcpy(&mProj, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetView(const DirectX::XMFLOAT4X4& data)
{
	memcpy(&mView, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetProj(const Matrix4& data)
{
	memcpy(&mProj, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetView(const Matrix4& data)
{
	memcpy(&mView, &data, sizeof(XMFLOAT4X4));
}


void Camera::Strafe(double d)
{
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, r, p));

	mViewDirty = true;
}

void Camera::Walk(double d)
{
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, l, p));

	mViewDirty = true;
}

void Camera::Pitch(double angle)
{
	// Rotate up and look vector about the right vector.

	Matrix4 R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::RotateY(double angle)
{
	// Rotate the basis vectors about the world y-axis.

	Matrix4 R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::UpdateProjectionMatrix()
{
	Matrix4 P = XMMatrixPerspectiveFovLH(mFovY, mAspect, mFarZ, mNearZ);
	XMStoreFloat4x4(&mProj, P);
}

void Camera::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&mRight);
	XMVECTOR U = XMLoadFloat3(&mUp);
	XMVECTOR L = XMLoadFloat3(&mLook);
	XMVECTOR P = XMLoadFloat3(&mPosition);

	mView = GetInverseTransformMatrix((Vector3&)R, (Vector3&)U, (Vector3&)L, (Vector3&)P);

	cameraToWorld[0] = R;
	cameraToWorld[1] = U;
	cameraToWorld[2] = L;
	cameraToWorld[3] = P;
	cameraToWorld[3].m128_f32[3] = 1;
}