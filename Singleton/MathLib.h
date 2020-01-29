#pragma once
#include "../Common/d3dUtil.h"

const float Deg2Rad = 0.0174532924;
const float Rad2Deg = 57.29578;
struct Cone
{
	DirectX::XMFLOAT3 vertex;
	float height;
	DirectX::XMFLOAT3 direction;
	float radius;
	Cone(DirectX::XMFLOAT3&& position, float distance, DirectX::XMFLOAT3&& direction, float angle) : 
		vertex(position),
		height(distance),
		direction(direction)
	{
		radius = tan(angle * 0.5) * height;
	}
};
class MathLib final
{
public:
	MathLib() = delete;
	~MathLib() = delete;
	static Math::Vector4 GetPlane(
		Math::Vector3&& normal,
		Math::Vector3&& inPoint);
	static Math::Vector4 GetPlane(
		Math::Vector3&& a,
		Math::Vector3&& b,
		Math::Vector3&& c);
	static bool BoxIntersect(
		const Math::Matrix4& localToWorldMatrix,
		Math::Vector4* planes,
		Math::Vector3&& position,
		Math::Vector3&& localExtent);
	static void GetCameraNearPlanePoints(
		Math::Matrix4&& localToWorldMatrix,
		double fov,
		double aspect,
		double distance,
		Math::Vector3* corners
	);

	static void GetPerspFrustumPlanes(
		Math::Matrix4&& localToWorldMatrix,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		DirectX::XMFLOAT4* frustumPlanes
	);
	static void GetFrustumBoundingBox(
		Math::Matrix4&& localToWorldMatrix,
		double nearWindowHeight,
		double farWindowHeight,
		double aspect,
		double nearZ,
		double farZ, 
		Math::Vector3* minValue,
		Math::Vector3* maxValue
	);

	static float GetDistanceToPlane(
		Math::Vector4&& plane,
		Math::Vector4&& point)
	{
		Math::Vector3 dotValue = DirectX::XMVector3Dot(plane, point);
		return ((DirectX::XMFLOAT4*)&dotValue)->x + ((DirectX::XMFLOAT4*)&point)->w;
	}
	static bool ConeIntersect(Cone&& cone, DirectX::XMVECTOR&& plane);
};