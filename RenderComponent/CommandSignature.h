#pragma once
#include "../Common/d3dUtil.h"
class Shader;
struct MultiDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
	//Size = 72 byte
	//int arr[18]
	MultiDrawCommand& operator=(const MultiDrawCommand& cmd);
	MultiDrawCommand& operator=(MultiDrawCommand&& cmd);
};

class CommandSignature
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> mCommandSignature;
	Shader* mShader;
public:
	CommandSignature(Shader* shader, ID3D12Device* device);
	ID3D12CommandSignature* GetSignature() const { return mCommandSignature.Get(); }
	Shader* GetShader() const { return mShader; }
};