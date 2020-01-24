#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
#include "../Common/RingQueue.h"
class FrameResource;
class RenderCommand
{
private:
	static std::mutex mtx;
	static RingQueue<RenderCommand*> queue;
public:
	virtual ~RenderCommand() {}
	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource) = 0;
	static void AddCommand(RenderCommand* command);
	static bool ExecuteCommand(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource);
};