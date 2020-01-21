#include "Component.h"
#include "Transform.h"
Component::Component(Transform* trans)
	: MObject(), transform(trans)
{
	if (trans != nullptr)
	{
		componentIndex = trans->allComponents.size();
		trans->allComponents.emplace_back(this);
	}
}

Component::~Component()
{
	if (transform != nullptr)
	{
		auto&& last = transform->allComponents.end() - 1;
		transform->allComponents[componentIndex] = *last;
		(*last)->componentIndex = componentIndex;
		transform->allComponents.erase(last);
	}
}