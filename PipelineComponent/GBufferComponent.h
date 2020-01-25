#pragma once
#include "PipelineComponent.h"
class GBufferRunnable;
class PSOContainer;
class StructuredBuffer;
class PrepareComponent;
class RenderTexture;
class SkyboxComponent;
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:
	std::vector<TemporalResourceCommand> tempRTRequire;
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
	RenderTexture* preintTexture;
	std::unique_ptr<PSOContainer> preintContainer;
public:
	SkyboxComponent* skboxComp;
	static UINT _AllLight;
	static UINT _LightIndexBuffer;
	static UINT LightCullCBuffer;
	static UINT TextureIndices;
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual CommandListType GetCommandListType()
	{
		return CommandListType_Graphics;
	}
};

