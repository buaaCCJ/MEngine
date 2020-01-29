#include "MathLib.h"
#include "../Common/MetaLib.h"
using namespace Math;
#define GetVec(name, v) XMVECTOR name = XMLoadFloat3(&##v);
#define StoreVec(ptr, v) XMStoreFloat3(ptr, v);
XMVECTOR mul(XMMATRIX& mat, XMVECTOR& vec)
{
	return XMVector4Transform(vec, XMMatrixTranspose(mat));
}
XMVECTOR mul(XMVECTOR& vec, XMMATRIX& mat)
{
	return XMVector4Transform(vec, mat);
}

Vector4 MathLib::GetPlane(
	Vector3&& a,
	Vector3&& b,
	Vector3&& c)
{
	XMVECTOR normal = XMVector3Normalize(XMVector3Cross(b - a, c - a));
	XMVECTOR disVec = -XMVector3Dot(normal, a);
	memcpy(&disVec, &normal, sizeof(XMFLOAT3));
	return disVec;
}
Vector4 MathLib::GetPlane(
	Vector3&& normal,
	Vector3&& inPoint)
{
	XMVECTOR dt = -XMVector3Dot(normal, inPoint);
	memcpy(&dt, &normal, sizeof(XMFLOAT3));
	return dt;
}
bool MathLib::BoxIntersect(const Matrix4& localToWorldMatrix, Vector4* planes, Vector3&& position, Vector3&& localExtent)
{
	XMMATRIX matrixTranspose = XMMatrixTranspose(localToWorldMatrix);
	XMVECTOR pos = XMVector3TransformCoord(position, matrixTranspose);
	auto func = [&](UINT i)->bool
	{
		XMVECTOR plane = planes[i];
		XMVECTOR absNormal = XMVectorAbs(XMVector3TransformNormal(plane, matrixTranspose));
		XMVECTOR result = XMVector3Dot(pos, plane) - XMVector3Dot(absNormal, localExtent);
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
	XMVECTOR farPoint = localToWorldMatrix[3] + distance * localToWorldMatrix[2];
	XMVECTOR upVec = upLength * localToWorldMatrix[1];
	XMVECTOR rightVec = rightLength * localToWorldMatrix[0];
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
	*(XMVECTOR*)frustumPlanes = GetPlane(std::move(localToWorldMatrix[2]), std::move(localToWorldMatrix[3] + farPlane * localToWorldMatrix[2]));
	*(XMVECTOR*)(frustumPlanes + 1) = GetPlane(-localToWorldMatrix[2], (localToWorldMatrix[3] + nearPlane * localToWorldMatrix[2]));
	*(XMVECTOR*)(frustumPlanes + 2) = GetPlane(std::move(nearCorners[1]), std::move(nearCorners[0]), std::move(localToWorldMatrix[3]));
	*(XMVECTOR*)(frustumPlanes + 3) = GetPlane(std::move(nearCorners[2]), std::move(nearCorners[3]), std::move(localToWorldMatrix[3]));
	*(XMVECTOR*)(frustumPlanes + 4) = GetPlane(std::move(nearCorners[0]), std::move(nearCorners[2]), std::move(localToWorldMatrix[3]));
	*(XMVECTOR*)(frustumPlanes + 5) = GetPlane(std::move(nearCorners[3]), std::move(nearCorners[1]), std::move(localToWorldMatrix[3]));
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
	XMVECTOR poses[8];
	XMVECTOR pos = localToWorldMatrix[3];
	XMVECTOR right = localToWorldMatrix[0];
	XMVECTOR up = localToWorldMatrix[1];
	XMVECTOR forward = localToWorldMatrix[2];
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

bool MathLib::ConeIntersect(Cone&& cone, XMVECTOR&& plane)
{
	XMVECTOR dir = XMLoadFloat3(&cone.direction);
	XMVECTOR vertex = XMLoadFloat3(&cone.vertex);
	XMVECTOR m = XMVector3Cross(XMVector3Cross(plane, dir), dir);
	XMVECTOR Q = vertex + dir * cone.height + XMVector3Normalize(m) * cone.radius;
	return (GetDistanceToPlane(std::move(vertex), std::move(plane)) < 0) || (GetDistanceToPlane(std::move(Q), std::move(plane)) < 0);
}