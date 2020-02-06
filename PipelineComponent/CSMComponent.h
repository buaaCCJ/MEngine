#pragma once
#include "PipelineComponent.h"
class CSMComponent : public PipelineComponent
{
private:
	std::vector<TemporalResourceCommand> tempResources;
public:
	virtual CommandListType GetCommandListType() { return CommandListType_Graphics; }
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
};