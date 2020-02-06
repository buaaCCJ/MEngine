#pragma once
#include "../Common/MObject.h"
#include "../Common/d3dUtil.h"
class Transform;
class Component : public MObject
{
	friend class Transform;
private:
	UINT componentIndex;
	bool enabled = false;
public:
	Transform* transform;
	Component(Transform* trans);
	bool GetEnabled() const
	{
		return enabled;
	}
	void SetEnabled(bool enabled)
	{
		if (this->enabled == enabled) return;
		this->enabled = enabled;
		if (enabled) OnEnable();
		else OnDisable();
	}
	virtual ~Component();
	virtual void Update() {}
	virtual void OnEnable() {}
	virtual void OnDisable() {}
};