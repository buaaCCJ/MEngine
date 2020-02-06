#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "Transform.h"
#include "World.h"
RandomVector<TransformData> Transform::randVec(100);
using namespace DirectX;
void Transform::SetRotation(XMVECTOR quaternion)
{
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
	rotationMatrix = XMMatrixTranspose(rotationMatrix);
	TransformData& data = randVec[vectorPos];
	XMStoreFloat3(&data.right, rotationMatrix.r[0]);
	XMStoreFloat3(&data.up, rotationMatrix.r[1]);
	XMStoreFloat3(&data.forward, rotationMatrix.r[2]);
}
void Transform::SetPosition(XMFLOAT3 position)
{
	randVec[vectorPos].position = position;
}

Transform::Transform(World* world) :
	MObject(), world(world)
{
	if (world != nullptr)
	{
		worldIndex = world->allTransformsPtr.size();
		world->allTransformsPtr.emplace_back(this);
	}
	else
	{
		worldIndex = -1;
	}
	randVec.Add(
		{
			{0,1,0},
			{0,0,1},
			{1,0,0},
			{1,1,1},
			{0,0,0}
		},
		&vectorPos
	);
}

void Transform::SetLocalScale(XMFLOAT3 localScale)
{
	randVec[vectorPos].localScale = localScale;
}

Math::Matrix4 Transform::GetLocalToWorldMatrix()
{
	Math::Matrix4 target;
	TransformData& data = randVec[vectorPos];
	XMVECTOR vec = XMLoadFloat3(&data.right);
	vec *= data.localScale.x;
	target[0] = vec;
	vec = XMLoadFloat3(&data.up);
	vec *= data.localScale.y;
	target[1] = vec;
	vec = XMLoadFloat3(&data.forward);
	vec *= data.localScale.z;
	target[2] = vec;
	target[3] = { data.position.x, data.position.y, data.position.z, 1 };
	return target;
}

Math::Matrix4 Transform::GetWorldToLocalMatrix()
{
	TransformData& data = randVec[vectorPos];
	return GetInverseTransformMatrix(
		(Math::Vector3)data.right * data.localScale.x,
		(Math::Vector3)data.up * data.localScale.y,
		(Math::Vector3)data.forward * data.localScale.z,
		data.position	);
}

Transform::~Transform()
{
	for (auto ite = allComponents.begin(); ite != allComponents.end(); ++ite)
	{
		(*ite)->transform = nullptr;
		delete *ite;
	}
	allComponents.clear();
	if (worldIndex >= 0)
	{
		auto&& ite = world->allTransformsPtr.end() - 1;
		(*ite)->worldIndex = worldIndex;
		world->allTransformsPtr[worldIndex] = *ite;
		world->allTransformsPtr.erase(ite);
	}
}