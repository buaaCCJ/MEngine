#include "CommandSignature.h"
#include "Shader.h"
#include "../Singleton/ShaderID.h"
CommandSignature::CommandSignature(Shader* shader, ID3D12Device* device) :
	mShader(shader)
{
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	D3D12_INDIRECT_ARGUMENT_DESC indDesc[4];
	ZeroMemory(indDesc, 4 * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[0].ConstantBufferView.RootParameterIndex = shader->GetPropertyRootSigPos(ShaderID::GetPerObjectBufferID());
	indDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	indDesc[1].VertexBuffer.Slot = 0;
	indDesc[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	indDesc[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	desc.ByteStride = sizeof(MultiDrawCommand);
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 4;
	desc.pArgumentDescs = indDesc;
	ThrowIfFailed(device->CreateCommandSignature(&desc, shader->GetSignature(), IID_PPV_ARGS(&mCommandSignature)));
}


MultiDrawCommand& MultiDrawCommand::operator=(const MultiDrawCommand& cmd)
{
	memcpy(this, &cmd, sizeof(MultiDrawCommand));
	return *this;
}

MultiDrawCommand& MultiDrawCommand::operator=(MultiDrawCommand&& cmd)
{
	memcpy(this, &cmd, sizeof(MultiDrawCommand));
	return *this;
}