#pragma once
#include <DirectXMath.h>
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
#include "../Common/RandomVector.h"
class World;
struct TransformData
{
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 localScale;
	DirectX::XMFLOAT3 position;
};
class Transform : public MObject
{
	friend class Component;
private:

	static RandomVector<TransformData> randVec;
	std::vector<Component*> allComponents;
	int worldIndex;
	UINT vectorPos;
	World* world;
public:
	DirectX::XMFLOAT3 GetPosition() const { return randVec[vectorPos].position; }
	DirectX::XMFLOAT3 GetForward() const { return randVec[vectorPos].forward; }
	DirectX::XMFLOAT3 GetRight() const { return randVec[vectorPos].right; }
	DirectX::XMFLOAT3 GetUp() const { return randVec[vectorPos].up; }
	DirectX::XMFLOAT3 GetLocalScale() const { return randVec[vectorPos].localScale; }
	void SetRotation(DirectX::XMVECTOR quaternion);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetLocalScale(DirectX::XMFLOAT3 localScale);
	DirectX::XMMATRIX GetLocalToWorldMatrix();
	DirectX::XMMATRIX GetWorldToLocalMatrix();
	Transform(World* world);
	~Transform();
};
