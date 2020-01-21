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
	static DirectX::XMVECTOR GetPlane(
		DirectX::XMVECTOR&& normal,
		DirectX::XMVECTOR&& inPoint);
	static DirectX::XMVECTOR GetPlane(
		DirectX::XMVECTOR&& a,
		DirectX::XMVECTOR&& b,
		DirectX::XMVECTOR&& c);
	static bool BoxIntersect(
		const DirectX::XMMATRIX& localToWorldMatrix,
		DirectX::XMVECTOR* planes,
		DirectX::XMVECTOR&& position,
		DirectX::XMVECTOR&& localExtent);
	static void GetCameraNearPlanePoints(
		DirectX::XMMATRIX&& localToWorldMatrix,
		double fov,
		double aspect,
		double distance,
		DirectX::XMVECTOR* corners
	);

	static void GetPerspFrustumPlanes(
		DirectX::XMMATRIX&& localToWorldMatrix,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		DirectX::XMFLOAT4* frustumPlanes
	);
	static void GetFrustumBoundingBox(
		DirectX::XMMATRIX&& localToWorldMatrix,
		double nearWindowHeight,
		double farWindowHeight,
		double aspect,
		double nearZ,
		double farZ, 
		DirectX::XMVECTOR* minValue,
		DirectX::XMVECTOR* maxValue
	);

	static float GetDistanceToPlane(
		DirectX::XMVECTOR&& plane,
		DirectX::XMVECTOR&& point)
	{
		DirectX::XMVECTOR dotValue = DirectX::XMVector3Dot(plane, point);
		return ((DirectX::XMFLOAT4*)&dotValue)->x + ((DirectX::XMFLOAT4*)&point)->w;
	}
	static bool ConeIntersect(Cone&& cone, DirectX::XMVECTOR&& plane);
	/*static bool ConeIntersect(Cone cone, DirectX::XMVECTOR plane)
	{
		float3 m = cross(cross(plane.xyz, cone.direction), cone.direction);
		float3 Q = cone.vertex + cone.direction * cone.height + normalize(m) * cone.radius;
		return PointInsidePlane(cone.vertex, plane) || PointInsidePlane(Q, plane);
	}*/
};