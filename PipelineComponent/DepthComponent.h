#pragma once 
#include "PipelineComponent.h"
class DepthComponent : public PipelineComponent
{
private:

public:
	std::vector<TemporalResourceCommand> tempRTRequire;
	virtual CommandListType GetCommandListType()
	{
		return CommandListType_Graphics;
	}
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
};