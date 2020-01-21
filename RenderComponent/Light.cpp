#include "Light.h"
#include "../LogicComponent/Transform.h"
#include "UploadBuffer.h"
#include "../Common/MetaLib.h"
#include "../Singleton/MathLib.h"
#include "../LogicComponent/World.h"
using namespace DirectX;
Light* Light::linkBegin = nullptr;
UINT Light::lightCount = 0;
Light::Light(Transform* trans) :
	Component(trans)
{


}

void Light::OnEnable()
{
	lightCount++;
	lastLinkNode = nullptr;
	nextLinkNode = linkBegin;
	linkBegin = this;
}

void Light::SetShadowEnabled(bool value) {
	isDirty = true;
	if (enableShadow != value)
	{
		enableShadow = value;
		if (value)
		{
			shadowIndex = World::GetInstance()->GetDescHeapIndexFromPool();
		}
		else
		{
			World::GetInstance()->ReturnDescHeapIndexToPool(shadowIndex);
			shadowIndex = -1;
		}
	}
}

void Light::OnDisable()
{
	lightCount--;
	if (lastLinkNode)
	{
		lastLinkNode->nextLinkNode = nextLinkNode;
	}
	if (nextLinkNode)
	{
		nextLinkNode->lastLinkNode = lastLinkNode;
	}
	lastLinkNode = nullptr;
	nextLinkNode = nullptr;
	if (shadowIndex >= 0)
	{
		World::GetInstance()->ReturnDescHeapIndexToPool(shadowIndex);
	}
}

void Light::GetLightingList(
	std::vector<LightCommand>& lightingResults,
	DirectX::XMVECTOR* frustumPlanes,
	DirectX::XMVECTOR&& frustumMinPoint,
	DirectX::XMVECTOR&& frustumMaxPoint)
{
	lightingResults.clear();
	for (Light* ite = linkBegin; ite; ite = ite->nextLinkNode)
	{
		XMVECTOR position = XMLoadFloat3(&ite->transform->GetPosition());
		XMVECTOR forward = XMLoadFloat3(&ite->transform->GetForward());
		XMVECTOR range = { ite->range, ite->range, ite->range, 0 };
		XMVECTOR minPoint = position - range;
		XMVECTOR maxPoint = position + range;
		if (XMVector3Greater(maxPoint, frustumMinPoint) &&
			XMVector3Less(minPoint, frustumMaxPoint)) {
			switch (ite->lightType)
			{
			case LightType_Spot:
			{
				Cone spotCone(ite->transform->GetPosition(), ite->range, ite->transform->GetForward(), ite->angle);
				auto func = [&](UINT i)->bool
				{
					return MathLib::ConeIntersect(std::move(spotCone), std::move(frustumPlanes[i]));
				};
				if (InnerLoopEarlyBreak<decltype(func), 6>(func))
				{
					lightingResults.push_back(ite->GetLightCommand(spotCone.radius));
				}
			}
			break;
			case LightType_Point:
			{
				auto func = [&](UINT i)->bool
				{
					return MathLib::GetDistanceToPlane(std::move(frustumPlanes[i]), std::move(position)) < ite->range;
				};
				if (InnerLoopEarlyBreak<decltype(func), 6>(func))
				{
					lightingResults.push_back(ite->GetLightCommand(0));
				}
			}
			break;
			}
		}
	}
}

LightCommand Light::GetLightCommand(float radius)
{
	return 
	{
		transform->GetForward(),
		enableShadow ? shadowIndex : -1,
		{color.x * intensity, color.y * intensity, color.z * intensity},
		(UINT)lightType,
		transform->GetPosition(),
		angle,
		shadowSoftValue,
		shadowBias,
		shadowNormalBias,
		range,
		radius		
	};
}