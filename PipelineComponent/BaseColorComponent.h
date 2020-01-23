#pragma once
#include "PipelineComponent.h"
class SkyboxComponent;
class RenderTexture;
class BaseColorRunnable;
class PSOContainer;
class BaseColorComponent : public PipelineComponent
{
private:
	friend class BaseColorRunnable;
	std::vector<TemporalResourceCommand> tempRTRequire;
	SkyboxComponent* skboxComp;
public:
	static UINT _AllLight;
	static UINT _LightIndexBuffer;
	static UINT LightCullCBuffer;
	static UINT TextureIndices;
	RenderTexture* preintTexture;
	std::unique_ptr<PSOContainer> preintContainer;
	virtual CommandListType GetCommandListType() { return CommandListType_Graphics; }
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
};