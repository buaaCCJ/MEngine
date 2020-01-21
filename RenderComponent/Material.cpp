#include "Material.h"
#include "MaterialManager.h"
#include "../Singleton/FrameResource.h"
Material::Material(MaterialManager* manager, ID3D12Device* device, FrameResource* resource) :
	manager(manager)
{
	cbufferEle = manager->GetElement(resource, device);
}

Material::~Material()
{
	manager->ReturnElement(cbufferEle);
}