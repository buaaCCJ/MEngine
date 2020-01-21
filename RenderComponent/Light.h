#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
#include <math.h>
class UploadBuffer;
class World;
struct LightCommand
{
	DirectX::XMFLOAT3 direction;
	int shadowmapIndex;
	//
	DirectX::XMFLOAT3 lightColor;
	UINT lightType;
	//Align
	DirectX::XMFLOAT3 position;
	float spotAngle;
	//Align
	float shadowSoftValue;
	float shadowBias;
	float shadowNormalBias;
	float range;
	//Align
	float spotRadius;
	DirectX::XMFLOAT3 __align;

};
class Light : public Component
{
public:
	enum LightType
	{
		LightType_Point = 0,
		LightType_Spot = 1
	};
private:
	static Light* linkBegin;
	static UINT lightCount;
	Light* lastLinkNode = nullptr;
	Light* nextLinkNode = nullptr;
	float intensity = 1;
	DirectX::XMFLOAT3 color = { 1,1,1 };
	LightType lightType = LightType_Point;
	bool isDirty = true;
	bool enableShadow = true;
	float shadowSoftValue = 0;
	float shadowBias = 0.05f;
	float shadowNormalBias = 0.4f;
	float shadowNearPlane = 0.2f;
	float range = 5;
	float angle = 60;
	int shadowIndex = -1;
public:
	Light(Transform* trans);
	virtual void OnEnable() override;
	virtual void OnDisable() override;
	static void GetLightingList(
		std::vector<LightCommand>& lightingResults,
		DirectX::XMVECTOR* frustumPlanes,
		DirectX::XMVECTOR&& frustumMinPoint, 
		DirectX::XMVECTOR&& frustumMaxPoint);
	float GetIntensity() const { return intensity; }
	float SetIntensity(float intensity) { 
		
		intensity = max(0, intensity);
		isDirty = true;
		this->intensity = intensity; 
	}
	float GetSpotAngle() const { return angle; }
	void SetSpotAngle(float angle)
	{
		isDirty = true;
		angle = max(1, angle);
		this->angle = angle;
	}
	DirectX::XMFLOAT3 GetColor() const { return color; }
	void SetColor(DirectX::XMFLOAT3 value)
	{
		isDirty = true;
		value.x = max(0, value.x);
		value.y = max(0, value.y);
		value.z = max(0, value.z);
		color = value;
	}
	bool ShadowEnabled() const { return enableShadow; }
	void SetShadowEnabled(bool value);
	float GetShadowSoftValue() const
	{
		return shadowSoftValue;
	}
	void SetShadowSoftValue(float value)
	{
		value = max(0, value);
		isDirty = true;
		shadowSoftValue = value;
	}
	float GetShadowBias() const
	{
		return shadowBias;
	}
	void SetShadowBias(float value)
	{
		isDirty = true;
		value = max(0.01f, value);
		shadowBias = value;
	}
	float GetShadowNormalBias() const
	{
		return shadowNormalBias;
	}
	void SetShadowNormalBias(float value)
	{
		isDirty = true;
		value = max(0.01f, value);
		shadowNormalBias = value;
	}
	float GetShadowNearPlane() const
	{
		return shadowNearPlane;
	}
	void SetShadowNearPlane(float value)
	{
		isDirty = true;
		value = max(0.1f, value);
		shadowNearPlane = value;
	}
	LightType GetLightType() const
	{
		return lightType;
	}
	void SetLightType(LightType type)
	{
		isDirty = true;
		lightType = type;
	}
	LightCommand GetLightCommand(float radius);
};

