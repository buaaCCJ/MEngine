#include "DirectionalLight.h"
#include "../RenderComponent/RenderTexture.h"
DirectionalLight* DirectionalLight::current = nullptr;
DirectionalLight::DirectionalLight(Transform* trans, uint* shadowmapResolution, ID3D12Device* device) :
	Component(trans)
{
	memcpy(this->shadowmapResolution, shadowmapResolution, sizeof(uint) * CascadeLevel);
	current = this;
	RenderTextureFormat smFormat;
	smFormat.depthFormat = RenderTextureDepthSettings_Depth16;
	smFormat.usage = RenderTextureUsage::RenderTextureUsage_DepthBuffer;
	for (uint i = 0; i < CascadeLevel; ++i)
	{
		shadowmaps[i] = new RenderTexture(
			device, shadowmapResolution[i], shadowmapResolution[i], smFormat, RenderTextureDimension_Tex2D, 1, 1, RenderTextureState::Generic_Read
		);
	}
}

DirectionalLight::~DirectionalLight()
{
	current = nullptr;
	for (uint i = 0; i < CascadeLevel; ++i)
	{
		shadowmaps[i].Destroy();
	}
}