#pragma once
#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
class Camera;
class PrepareRunnable;
class MeshRenderer;
class PrepareComponent : public PipelineComponent
{
	friend class PrepareRunnable;
public:
	PassConstants passConstants;
	DirectX::XMFLOAT4 frustumPlanes[6];
	Math::Vector3 frustumMinPos;
	Math::Vector3 frustumMaxPos;
	DirectX::XMFLOAT4 _ZBufferParams;
protected:
	std::vector<TemporalResourceCommand> useless;
	virtual CommandListType GetCommandListType() {
		return CommandListType_None;
	}
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(EventData& evt) { return useless; }
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {

	};
	virtual void Dispose() {};
};
