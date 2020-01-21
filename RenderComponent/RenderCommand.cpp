#include "RenderCommand.h"
#include "../Singleton/FrameResource.h"
std::mutex RenderCommand::mtx;
RingQueue<std::shared_ptr<RenderCommand>> RenderCommand::queue(100);

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
	std::shared_ptr<RenderCommand> cmd = nullptr;
	bool v = false;
	{
		std::lock_guard<std::mutex> lck(mtx);
		v = queue.TryPop(&cmd);
	}
	if (!v) return false;
	RenderCommand* ptr = cmd.operator->();
	(*cmd)(device, commandList, resource);
	return true;
}