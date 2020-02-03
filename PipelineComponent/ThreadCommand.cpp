#include "ThreadCommand.h"
#include "../RenderComponent/RenderTexture.h"
#include "../RenderComponent/StructuredBuffer.h"
#include "../Singleton/Graphics.h"
void ThreadCommand::ResetCommand()
{
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
}
void ThreadCommand::CloseCommand()
{
	for (auto ite = rtStateMap.begin(); ite != rtStateMap.end(); ++ite)
	{
		if (ite->second)
		{
			D3D12_RESOURCE_STATES writeState, readState;
			writeState = ite->first->GetInitState();
			if (ite->first->GetUsage() == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
			{
				readState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
			else
			{
				readState = D3D12_RESOURCE_STATE_DEPTH_READ;
			}
			Graphics::ResourceStateTransform(cmdList.Get(), readState, writeState, ite->first->GetColorResource());
		}
	}
	for (auto ite = sbufferStateMap.begin(); ite != sbufferStateMap.end(); ++ite)
	{
		if (ite->second)
		{
			Graphics::ResourceStateTransform(cmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, ite->first->GetResource());
		}
	}
	rtStateMap.clear();
	sbufferStateMap.clear();
	cmdList->Close();
}
ThreadCommand::ThreadCommand(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	rtStateMap.reserve(20);
	sbufferStateMap.reserve(20);
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

void ThreadCommand::SetResourceReadWriteState(RenderTexture* rt, ResourceReadWriteState state)
{
	auto ite = rtStateMap.find(rt);
	if (state)
	{
		if (ite == rtStateMap.end())
		{
			rtStateMap.insert_or_assign(rt, state);
		}
		else if (ite->second != state)
		{
			ite->second = state;
		}
		else
			return;
	}
	else
	{
		if (ite != rtStateMap.end())
		{
			rtStateMap.erase(ite);
		}
		else
			return;
	}
	D3D12_RESOURCE_STATES writeState, readState;
	writeState = rt->GetInitState();
	if (rt->GetUsage() == RenderTextureUsage::RenderTextureUsage_ColorBuffer)
	{
		readState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else
	{
		readState = D3D12_RESOURCE_STATE_DEPTH_READ;
	}
	if (state)
	{
		Graphics::ResourceStateTransform(cmdList.Get(), writeState, readState, rt->GetColorResource());
	}
	else
	{
		Graphics::ResourceStateTransform(cmdList.Get(), readState, writeState, rt->GetColorResource());
	}
}

void ThreadCommand::SetResourceReadWriteState(StructuredBuffer* rt, ResourceReadWriteState state)
{
	auto ite = sbufferStateMap.find(rt);
	if (state)
	{
		if (ite == sbufferStateMap.end())
		{
			sbufferStateMap.insert_or_assign(rt, state);
		}
		else if (ite->second != state)
		{
			ite->second = state;
		}
		else
			return;
	}
	else
	{
		if (ite != sbufferStateMap.end())
		{
			sbufferStateMap.erase(ite);
		}
		else
			return;
	}
	if (state)
	{
		Graphics::ResourceStateTransform(cmdList.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, rt->GetResource());
	}
	else
	{
		Graphics::ResourceStateTransform(cmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, rt->GetResource());
	}
}