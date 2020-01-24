#include "RenderCommand.h"
#include "../Singleton/FrameResource.h"
std::mutex RenderCommand::mtx;
RingQueue<RenderCommand*> RenderCommand::queue(100);

void RenderCommand::AddCommand(RenderCommand* command)
{
	std::lock_guard<std::mutex> lck(mtx);
	queue.EmplacePush(command);
}

bool RenderCommand::ExecuteCommand(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource)
{
	RenderCommand* ptr = nullptr;
	bool v = false;
	{
		std::lock_guard<std::mutex> lck(mtx);
		v = queue.TryPop(&ptr);
	}
	if (!v) return false;
	PointerKeeper<RenderCommand> keeper(ptr);
	(*ptr)(device, commandList, resource);
	return true;
}