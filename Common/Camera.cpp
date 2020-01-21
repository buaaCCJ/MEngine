//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Camera.h"
#include "../RenderComponent/CBufferPool.h"
#include "../RenderComponent/UploadBuffer.h"
using namespace DirectX;

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
	XMMATRIX view = GetView();
	XMMATRIX proj = GetProj();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, (view));
	XMStoreFloat4x4(&mMainPassCB.InvView, (invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, (proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, (invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, (viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, (invViewProj));
	mMainPassCB.NearZ = GetNearZ();
	mMainPassCB.worldSpaceCameraPos = GetPosition3f();
	mMainPassCB.FarZ = GetFarZ();
	//auto& currPassCB = res->cameraCBs[GetInstanceID()];
	//currPassCB.buffer->CopyData(currPassCB.element, &mMainPassCB);
}

XMVECTOR Camera::GetPosition()const
{
	return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Camera::GetPosition3f()const
{
	return mPosition;
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

XMVECTOR Camera::GetRight()const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera::GetRight3f()const
{
	return mRight;
}

XMVECTOR Camera::GetUp()const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera::GetUp3f()const
{
	return mUp;
}

XMVECTOR Camera::GetLook()const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera::GetLook3f()const
{
	return mLook;
}

double Camera::GetNearZ()const
{
	return mNearZ;
}

double Camera::GetFarZ()const
{
	return mFarZ;
}

double Camera::GetAspect()const
{
	return mAspect;
}

double Camera::GetFovY()const
{
	return mFovY;
}

double Camera::GetFovX()const
{
	double halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f*atan(halfWidth / mNearZ);
}

double Camera::GetNearWindowWidth()const
{
	return mAspect * mNearWindowHeight;
}

double Camera::GetNearWindowHeight()const
{
	return mNearWindowHeight;
}

double Camera::GetFarWindowWidth()const
{
	return mAspect * mFarWindowHeight;
}

double Camera::GetFarWindowHeight()const
{
	return mFarWindowHeight;
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

XMMATRIX Camera::GetView()const
{
	return *(XMMATRIX*)(&mView);
}

XMMATRIX Camera::GetProj()const
{
	return *(XMMATRIX*)(&mProj);
}


XMFLOAT4X4 Camera::GetView4x4f()const
{
	return mView;
}

void Camera::SetProj(const DirectX::XMFLOAT4X4& data)
{
	memcpy(&mProj, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetView(const DirectX::XMFLOAT4X4& data)
{
	memcpy(&mView, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetProj(const DirectX::XMMATRIX& data)
{
	memcpy(&mProj, &data, sizeof(XMFLOAT4X4));
}
void Camera::SetView(const DirectX::XMMATRIX& data)
{
	memcpy(&mView, &data, sizeof(XMFLOAT4X4));
}


XMFLOAT4X4 Camera::GetProj4x4f()const
{
	return mProj;
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

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::RotateY(double angle)
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::UpdateProjectionMatrix()
{
	XMMATRIX P = XMMatrixPerspectiveFovLH(mFovY, mAspect, mFarZ, mNearZ);
	XMStoreFloat4x4(&mProj, P);
}

void Camera::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&mRight);
	XMVECTOR U = XMLoadFloat3(&mUp);
	XMVECTOR L = XMLoadFloat3(&mLook);
	XMVECTOR P = XMLoadFloat3(&mPosition);

	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L);

	// Fill in the view matrix entries.
	double x = -XMVectorGetX(XMVector3Dot(P, R));
	double y = -XMVectorGetX(XMVector3Dot(P, U));
	double z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLook, L);

	mView(0, 0) = mRight.x;
	mView(1, 0) = mRight.y;
	mView(2, 0) = mRight.z;
	mView(3, 0) = x;

	mView(0, 1) = mUp.x;
	mView(1, 1) = mUp.y;
	mView(2, 1) = mUp.z;
	mView(3, 1) = y;

	mView(0, 2) = mLook.x;
	mView(1, 2) = mLook.y;
	mView(2, 2) = mLook.z;
	mView(3, 2) = z;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
}

