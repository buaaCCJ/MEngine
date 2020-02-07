#pragma once
#include "Component.h"
class Transform;
class RenderTexture;
class DirectionalLight final : public Component
{
public:
	static const uint CascadeLevel = 4;
private:
	static DirectionalLight* current;
	uint shadowmapResolution[CascadeLevel];
	ObjectPtr<RenderTexture> shadowmaps[CascadeLevel];
	DirectionalLight(
		Transform* trans,
		uint* shadowmapResolution,
		ID3D12Device* device);

	~DirectionalLight();
public:
	static void DestroyLight()
	{
		if (current)
		{
			delete current;
		}
		current = nullptr;
	}
	static DirectionalLight* GetInstance(
		Transform* trans,
		uint* shadowmapResolution,
		ID3D12Device* device)
	{
		if (!current) current = new DirectionalLight(trans, shadowmapResolution, device);
		return current;
	}
	static DirectionalLight* GetInstance()
	{
		return current;
	}
	float intensity = 1;
	float3 color = { 1,1,1 };
	float shadowDistance[CascadeLevel] = { 7,15,50,150 };
	float shadowSoftValue[CascadeLevel] = { 0,0,0,0 };
	float shadowBias[CascadeLevel] = { 0.05f, 0.05f, 0.05f, 0.05f };
	constexpr uint GetShadowmapResolution(uint level) const
	{
#ifndef NDEBUG
		if (level >= CascadeLevel) throw "Out of Range Exception";
#endif
		return shadowmapResolution[level];
	}
	ObjectPtr<RenderTexture>& GetShadowmap(uint level)
	{
#ifndef NDEBUG
		if (level >= CascadeLevel) throw "Out of Range Exception";
#endif
		return shadowmaps[level];
	}

};