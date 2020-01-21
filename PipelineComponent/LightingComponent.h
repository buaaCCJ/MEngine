#pragma once
#include "PipelineComponent.h"
class PrepareComponent;
class LightFrameData : public IPipelineResource
{
public:
	UploadBuffer lightsInFrustum;
	UploadBuffer lightCBuffer;
	LightFrameData(ID3D12Device* device);
};

class LightingComponent : public PipelineComponent
{
private:
	std::vector<TemporalResourceCommand> tempResources;
	PrepareComponent* prepareComp;
public:
	std::unique_ptr<RenderTexture> xyPlaneTexture;
	std::unique_ptr<RenderTexture> zPlaneTexture;
	std::unique_ptr<StructuredBuffer> lightIndexBuffer;
	std::unique_ptr<DescriptorHeap> cullingDescHeap;
	virtual CommandListType GetCommandListType() { return CommandListType_Compute; }
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
};