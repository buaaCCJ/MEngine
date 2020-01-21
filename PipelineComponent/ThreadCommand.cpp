#include "ThreadCommand.h"
void ThreadCommand::ResetCommand()
{
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
}
void ThreadCommand::CloseCommand()
{
	cmdList->Close();
}
ThreadCommand::ThreadCommand(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		type,
		IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(device->CreateCommandList(
		0,
		type,
		cmdAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(&cmdList)));
	cmdList->Close();
}
