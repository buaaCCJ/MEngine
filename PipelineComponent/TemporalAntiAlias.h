#pragma once
#include "PipelineComponent.h"
class TemporalAntiAlias	: public PipelineComponent
{
protected:
	std::vector<TemporalResourceCommand> tempRT;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};