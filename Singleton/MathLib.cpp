#include "MathLib.h"
#include "../Common/MetaLib.h"
using namespace DirectX;
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

XMVECTOR MathLib::GetPlane(
	XMVECTOR&& a,
	XMVECTOR&& b,
	XMVECTOR&& c)
{
	XMVECTOR normal = XMVector3Normalize(XMVector3Cross(b - a, c - a));
	XMVECTOR disVec = -XMVector3Dot(normal, a);
	memcpy(&disVec, &normal, sizeof(XMFLOAT3));
	return disVec;
}
XMVECTOR MathLib::GetPlane(
	XMVECTOR&& normal,
	XMVECTOR&& inPoint)
{
	XMVECTOR dt = -XMVector3Dot(normal, inPoint);
	memcpy(&dt, &normal, sizeof(XMFLOAT3));
	return dt;
}
bool MathLib::BoxIntersect(const XMMATRIX& localToWorldMatrix, XMVECTOR* planes, XMVECTOR&& position, XMVECTOR&& localExtent)
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
	XMMATRIX&& localToWorldMatrix,
	double fov,
	double aspect,
	double distance,
	XMVECTOR* corners
)
{
	double upLength = distance * tan(fov * 0.5);
	double rightLength = upLength * aspect;
	XMVECTOR farPoint = localToWorldMatrix.r[3] + distance * localToWorldMatrix.r[2];
	XMVECTOR upVec = upLength * localToWorldMatrix.r[1];
	XMVECTOR rightVec = rightLength * localToWorldMatrix.r[0];
	corners[0] = farPoint - upVec - rightVec;
	corners[1] = farPoint - upVec + rightVec;
	corners[2] = farPoint + upVec - rightVec;
	corners[3] = farPoint + upVec + rightVec;
}

void MathLib::GetPerspFrustumPlanes(
	XMMATRIX&& localToWorldMatrix,
	double fov,
	double aspect,
	double nearPlane,
	double farPlane,
	XMFLOAT4* frustumPlanes
)
{
	XMVECTOR nearCorners[4];
	GetCameraNearPlanePoints(std::move(localToWorldMatrix), fov, aspect, nearPlane, nearCorners);
	*(XMVECTOR*)frustumPlanes = GetPlane(std::move(localToWorldMatrix.r[2]), std::move(localToWorldMatrix.r[3] + farPlane * localToWorldMatrix.r[2]));
	*(XMVECTOR*)(frustumPlanes + 1) = GetPlane(-localToWorldMatrix.r[2], (localToWorldMatrix.r[3] + nearPlane * localToWorldMatrix.r[2]));
	*(XMVECTOR*)(frustumPlanes + 2) = GetPlane(std::move(nearCorners[1]), std::move(nearCorners[0]), std::move(localToWorldMatrix.r[3]));
	*(XMVECTOR*)(frustumPlanes + 3) = GetPlane(std::move(nearCorners[2]), std::move(nearCorners[3]), std::move(localToWorldMatrix.r[3]));
	*(XMVECTOR*)(frustumPlanes + 4) = GetPlane(std::move(nearCorners[0]), std::move(nearCorners[2]), std::move(localToWorldMatrix.r[3]));
	*(XMVECTOR*)(frustumPlanes + 5) = GetPlane(std::move(nearCorners[3]), std::move(nearCorners[1]), std::move(localToWorldMatrix.r[3]));
}

void MathLib::GetFrustumBoundingBox(
	XMMATRIX&& localToWorldMatrix,
	double nearWindowHeight,
	double farWindowHeight,
	double aspect,
	double nearZ,
	double farZ,
	XMVECTOR* minValue,
	XMVECTOR* maxValue)
{
	double halfNearYHeight = nearWindowHeight * 0.5;
	double halfFarYHeight = farWindowHeight * 0.5;
	double halfNearXWidth = halfNearYHeight * aspect;
	double halfFarXWidth = halfFarYHeight * aspect;
	XMVECTOR poses[8];
	XMVECTOR pos = localToWorldMatrix.r[3];
	XMVECTOR right = localToWorldMatrix.r[0];
	XMVECTOR up = localToWorldMatrix.r[1];
	XMVECTOR forward = localToWorldMatrix.r[2];
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