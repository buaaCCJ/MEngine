//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Simple first person style camera class that lets the viewer explore the 3D scene.
//   -It keeps track of the camera coordinate system relative to the world space
//    so that the view matrix can be constructed.  
//   -It keeps track of the viewing frustum of the camera so that the projection
//    matrix can be obtained.
//***************************************************************************************

#ifndef CAMERA_H
#define CAMERA_H

#include "d3dUtil.h"
#include "../Common/MObject.h"
#include "../Singleton/FrameResource.h"
#include "../PipelineComponent/IPerCameraResource.h"
class PipelineComponent;
class RenderTexture;
class Camera : public MObject
{
	friend class PipelineComponent;
public:
	enum CameraRenderPath
	{
		DefaultPipeline = 0,
		Depth = 1
	};
	RenderTexture* renderTarget = nullptr;
	virtual ~Camera();
	Camera(ID3D12Device* device, CameraRenderPath rtType);
	// Get/Set world camera position.
	Math::Vector3 GetPosition() const { return mPosition; }
	void SetPosition(double x, double y, double z);
	void SetPosition(const DirectX::XMFLOAT3& v);

	// Get camera basis vectors.
	Math::Vector3 GetRight()const { return mRight; }
	Math::Vector3 GetUp()const { return mUp; }
	Math::Vector3 GetLook()const { return mLook; }

	// Get frustum properties.
	double GetNearZ()const { return mNearZ; }
	double GetFarZ()const { return mFarZ; }
	double GetAspect()const { return mAspect; }
	double GetFovY()const { return mFovY; }
	double GetFovX()const {
		double halfWidth = 0.5*GetNearWindowWidth();
		return 2.0*atan(halfWidth / mNearZ);
	}

	// Get near and far plane dimensions in view space coordinates.
	double GetNearWindowWidth()const { return mAspect * mNearWindowHeight; }
	double GetNearWindowHeight()const { return mNearWindowHeight; }
	double GetFarWindowWidth()const { return mAspect * mFarWindowHeight; }
	double GetFarWindowHeight()const { return mFarWindowHeight; }

	// Set frustum.
	void SetLens(double fovY, double aspect, double zn, double zf);

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// Get View/Proj matrices.
	Math::Matrix4 GetView()const { return mView; }
	Math::Matrix4 GetCameraToWorld() const { return cameraToWorld; }
	Math::Matrix4 GetProj()const { return mProj; }
	void SetProj(const DirectX::XMFLOAT4X4& data);
	void SetProj(const Math::Matrix4& data);
	void SetView(const DirectX::XMFLOAT4X4& data);
	void SetView(const Math::Matrix4& data);
	// Strafe/Walk the camera a distance d.
	void Strafe(double d);
	void Walk(double d);

	// Rotate the camera.
	void Pitch(double angle);
	void RotateY(double angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	void UploadCameraBuffer(PassConstants& mMainPassCB);
	CameraRenderPath GetRenderingPath() const { return renderType; }
	template <typename Func>
	inline IPipelineResource* GetResource(void* targetComponent, const Func& func)
	{
		std::lock_guard<std::mutex> lck(mtx);
		auto&& ite = perCameraResource.find(targetComponent);
		if (ite == perCameraResource.end())
		{
			IPipelineResource* newComp = func();
			perCameraResource.insert_or_assign(targetComponent, newComp);
			return newComp;
		}
		return ite->second;
	}

private:
	std::unordered_map<void*, IPipelineResource*> perCameraResource;
	std::mutex mtx;
	CameraRenderPath renderType;	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	double mNearZ = 0.0f;
	double mFarZ = 0.0f;
	double mAspect = 0.0f;
	double mFovY = 0.0f;
	double mNearWindowHeight = 0.0f;
	double mFarWindowHeight = 0.0f;


	bool mViewDirty = true;

	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	Math::Matrix4 cameraToWorld;
};

#endif