#include "CommandBuffer.h"


void CommandBuffer::Clear()
{
	executeCommands.clear();
	graphicsCmdLists.clear();

}
void CommandBuffer::Submit()
{
	uint startPos = 0;
	uint commandType = 2;
	uint executeCommandCount = 0;
	auto executeCmdList = [&](uint i)->void
	{
		if (commandType != i)
		{
			if (executeCommandCount > 0)
			{
				if (commandType == 0)
				{
					graphicsCommandQueue->ExecuteCommandLists(executeCommandCount, (ID3D12CommandList**)(graphicsCmdLists.data() + startPos));
				}
				else if (commandType == 1)
				{
					computeCommandQueue->ExecuteCommandLists(executeCommandCount, (ID3D12CommandList**)(graphicsCmdLists.data() + startPos));
				}
				startPos += executeCommandCount;
				executeCommandCount = 0;
			}
			commandType = i;
		}
		if (i < 2)
		{
			executeCommandCount++;
		}
	};
	for (auto ite = executeCommands.begin(); ite != executeCommands.end(); ++ite)
	{
		switch (ite->type)
		{
		case InnerCommand::CommandType_ExecuteGraphics:
			executeCmdList(0);
			break;
		case InnerCommand::CommandType_ExecuteCompute:
			executeCmdList(1);
			break;
		default:
			executeCmdList(2);
			break;
		}
		switch(ite->type)
		{
		case InnerCommand::CommandType_WaitGraphics:
			graphicsCommandQueue->Wait(ite->waitFence.fence, ite->waitFence.frameIndex);
			break;
		case InnerCommand::CommandType_SignalCompute:
			computeCommandQueue->Signal(ite->signalFence.fence, ite->signalFence.frameIndex);
			break;
		case InnerCommand::CommandType_WaitCompute:
			computeCommandQueue->Wait(ite->waitFence.fence, ite->waitFence.frameIndex);
			break;
		case InnerCommand::CommandType_SignalGraphics:
			graphicsCommandQueue->Signal(ite->signalFence.fence, ite->signalFence.frameIndex);
			break;
		}
	}
	executeCmdList(2);
}

void CommandBuffer::WaitForCompute(ID3D12Fence* computeFence, UINT currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_WaitCompute;
	cmd.waitFence.fence = computeFence;
	cmd.waitFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::WaitForGraphics(ID3D12Fence* computeFence, UINT currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_WaitGraphics;
	cmd.waitFence.fence = computeFence;
	cmd.waitFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::SignalToCompute(ID3D12Fence* computeFence, UINT currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_SignalCompute;
	cmd.signalFence.fence = computeFence;
	cmd.signalFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::SignalToGraphics(ID3D12Fence* computeFence, UINT currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_SignalGraphics;
	cmd.signalFence.fence = computeFence;
	cmd.signalFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::ExecuteGraphicsCommandList(ID3D12GraphicsCommandList* commandList)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_ExecuteGraphics;
	cmd.executeIndex = graphicsCmdLists.size();
	graphicsCmdLists.push_back(commandList);
	executeCommands.push_back(cmd);

}
void CommandBuffer::ExecuteComputeCommandList(ID3D12GraphicsCommandList* commandList)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_ExecuteCompute;
	cmd.executeIndex = graphicsCmdLists.size();
	graphicsCmdLists.push_back(commandList);
	executeCommands.push_back(cmd);
}

CommandBuffer::CommandBuffer(
	ID3D12CommandQueue* graphicsCommandQueue,
	ID3D12CommandQueue* computeCommandQueue
) : graphicsCommandQueue(graphicsCommandQueue),
computeCommandQueue(computeCommandQueue)
{
	graphicsCmdLists.reserve(20);
}