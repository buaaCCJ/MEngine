#include "MathLib.h"
#include "../Common/MetaLib.h"
using namespace Math;
#define GetVec(name, v) Vector4 name = XMLoadFloat3(&##v);
#define StoreVec(ptr, v) XMStoreFloat3(ptr, v);

Vector4 MathLib::GetPlane(
	const Vector3& a,
	const Vector3& b,
	const Vector3& c)
{
	Vector3 normal = normalize(cross(b - a, c - a));
	float disVec = -dot(normal, a);
	Vector4& v = (Vector4&)normal;
	v.SetW(disVec);
	return v;
}
Vector4 MathLib::GetPlane(
	const Vector3& normal,
	const Vector3& inPoint)
{
	float dt = -dot(normal, inPoint);
	Vector4 result = normal;
	result.SetW(dt);
	return result;
}
bool MathLib::BoxIntersect(const Matrix4& localToWorldMatrix, Vector4* planes, Vector3&& position, Vector3&& localExtent)
{
	XMMATRIX matrixTranspose = XMMatrixTranspose(localToWorldMatrix);
	Vector4 pos = XMVector3TransformCoord(position, matrixTranspose);
	auto func = [&](UINT i)->bool
	{
		Vector4 plane = planes[i];
		Vector4 absNormal = XMVectorAbs(XMVector3TransformNormal(plane, matrixTranspose));
		Vector4 result = XMVector3Dot(pos, plane) - XMVector3Dot(absNormal, localExtent);
		float dist = 0; XMFLOAT4 planeF;
		XMStoreFloat(&dist, result);
		XMStoreFloat4(&planeF, plane);
		if (dist > -planeF.w) return false;
	};
	InnerLoopEarlyBreak<decltype(func), 6>(func);
	return true;
}

void MathLib::GetCameraNearPlanePoints(
	Matrix4&& localToWorldMatrix,
	double fov,
	double aspect,
	double distance,
	Vector3* corners
)
{
	double upLength = distance * tan(fov * 0.5);
	double rightLength = upLength * aspect;
	Vector4 farPoint = localToWorldMatrix[3] + distance * localToWorldMatrix[2];
	Vector4 upVec = upLength * localToWorldMatrix[1];
	Vector4 rightVec = rightLength * localToWorldMatrix[0];
	corners[0] = farPoint - upVec - rightVec;
	corners[1] = farPoint - upVec + rightVec;
	corners[2] = farPoint + upVec - rightVec;
	corners[3] = farPoint + upVec + rightVec;
}

void MathLib::GetPerspFrustumPlanes(
	Matrix4&& localToWorldMatrix,
	double fov,
	double aspect,
	double nearPlane,
	double farPlane,
	XMFLOAT4* frustumPlanes
)
{
	Vector3 nearCorners[4];
	GetCameraNearPlanePoints(std::move(localToWorldMatrix), fov, aspect, nearPlane, nearCorners);
	*(Vector4*)frustumPlanes = GetPlane(std::move(localToWorldMatrix[2]), std::move(localToWorldMatrix[3] + farPlane * localToWorldMatrix[2]));
	*(Vector4*)(frustumPlanes + 1) = GetPlane(-localToWorldMatrix[2], (localToWorldMatrix[3] + nearPlane * localToWorldMatrix[2]));
	*(Vector4*)(frustumPlanes + 2) = GetPlane(std::move(nearCorners[1]), std::move(nearCorners[0]), std::move(localToWorldMatrix[3]));
	*(Vector4*)(frustumPlanes + 3) = GetPlane(std::move(nearCorners[2]), std::move(nearCorners[3]), std::move(localToWorldMatrix[3]));
	*(Vector4*)(frustumPlanes + 4) = GetPlane(std::move(nearCorners[0]), std::move(nearCorners[2]), std::move(localToWorldMatrix[3]));
	*(Vector4*)(frustumPlanes + 5) = GetPlane(std::move(nearCorners[3]), std::move(nearCorners[1]), std::move(localToWorldMatrix[3]));
}

void MathLib::GetFrustumBoundingBox(
	Matrix4&& localToWorldMatrix,
	double nearWindowHeight,
	double farWindowHeight,
	double aspect,
	double nearZ,
	double farZ,
	Vector3* minValue,
	Vector3* maxValue)
{
	double halfNearYHeight = nearWindowHeight * 0.5;
	double halfFarYHeight = farWindowHeight * 0.5;
	double halfNearXWidth = halfNearYHeight * aspect;
	double halfFarXWidth = halfFarYHeight * aspect;
	Vector4 poses[8];
	Vector4 pos = localToWorldMatrix[3];
	Vector4 right = localToWorldMatrix[0];
	Vector4 up = localToWorldMatrix[1];
	Vector4 forward = localToWorldMatrix[2];
	poses[0] = pos + forward * nearZ - right * halfNearXWidth - up * halfNearYHeight;
	poses[1] = pos + forward * nearZ - right * halfNearXWidth + up * halfNearYHeight;
	poses[2] = pos + forward * nearZ + right * halfNearXWidth - up * halfNearYHeight;
	poses[3] = pos + forward * nearZ + right * halfNearXWidth + up * halfNearYHeight;
	poses[4] = pos + forward * farZ - right * halfFarXWidth - up * halfFarYHeight;
	poses[5] = pos + forward * farZ - right * halfFarXWidth + up * halfFarYHeight;
	poses[6] = pos + forward * farZ + right * halfFarXWidth - up * halfFarYHeight;
	poses[7] = pos + forward * farZ + right * halfFarXWidth + up * halfFarYHeight;
	*minValue = poses[7];
	*maxValue = poses[7];
	auto func = [&](UINT i)->void
	{
		*minValue = XMVectorMin(poses[i], *minValue);
		*maxValue = XMVectorMax(poses[i], *maxValue);
	};
	InnerLoop<decltype(func), 7>(func);
}

bool MathLib::ConeIntersect(Cone&& cone, Vector4&& plane)
{
	Vector4 dir = XMLoadFloat3(&cone.direction);
	Vector4 vertex = XMLoadFloat3(&cone.vertex);
	Vector4 m = XMVector3Cross(XMVector3Cross(plane, dir), dir);
	Vector4 Q = vertex + dir * cone.height + (Vector4)XMVector3Normalize(m) * cone.radius;
	return (GetDistanceToPlane(std::move(vertex), std::move(plane)) < 0) || (GetDistanceToPlane(std::move(Q), std::move(plane)) < 0);
}

void MathLib::GetOrthoCamFrustumPlanes(
	const Math::Vector3& right,
	const Math::Vector3& up,
	const Math::Vector3& forward,
	const Math::Vector3& position,
	float xSize,
	float ySize,
	float nearPlane,
	float farPlane,
	Vector4* results)
{
	Vector3 normals[6];
	Vector3 positions[6];
	normals[0] = up;
	positions[0] = position + up * ySize;
	normals[1] = -up;
	positions[1] = position - up * ySize;
	normals[2] = right;
	positions[2] = position + right * xSize;
	normals[3] = -right;
	positions[3] = position - right * xSize;
	normals[4] = forward;
	positions[4] = position + forward * farPlane;
	normals[5] = -forward;
	positions[5] = position + forward * nearPlane;
	auto func = [&](uint i)->void
	{
		results[i] = GetPlane(normals[i], positions[i]);
	};
	InnerLoop<decltype(func), 6>(func);
}
void MathLib::GetOrthoCamFrustumPoints(
	const Math::Vector3& right,
	const Math::Vector3& up,
	const Math::Vector3& forward,
	const Math::Vector3& position,
	float xSize,
	float ySize,
	float nearPlane,
	float farPlane,
	Vector3* results)
{
	results[0] = position + xSize * right + ySize * up + farPlane * forward;
	results[1] = position + xSize * right + ySize * up + nearPlane * forward;
	results[2] = position + xSize * right - ySize * up + farPlane * forward;
	results[3] = position + xSize * right - ySize * up + nearPlane * forward;
	results[4] = position - xSize * right + ySize * up + farPlane * forward;
	results[5] = position - xSize * right + ySize * up + nearPlane * forward;
	results[6] = position - xSize * right - ySize * up + farPlane * forward;
	results[7] = position - xSize * right - ySize * up + nearPlane * forward;
}