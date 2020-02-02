#pragma once
#include "../PipelineComponent.h"
#include "../../Common/d3dUtil.h"
#include "../../Singleton/ShaderID.h"
#include "../../Singleton/Graphics.h"
#include "../../Singleton/ShaderCompiler.h"
using namespace Math;
struct MotionBlurCBufferData
{
	float4 _ZBufferParams;
	float4 _NeighborMaxTex_TexelSize;
	float4 _VelocityTex_TexelSize;
	float4 _MainTex_TexelSize;
	float2 _ScreenParams;
	float _VelocityScale;
	float _MaxBlurRadius;
	float _RcpMaxBlurRadius;
	float _TileMaxLoop;
	float _LoopCount;
	float _TileMaxOffs;
};
class MotionBlurFrameData : public IPipelineResource
{
public:
	UploadBuffer constParams;
	DescriptorHeap descHeap;
	MotionBlurFrameData(ID3D12Device* device) :
		constParams(device, 7, true, sizeof(MotionBlurCBufferData)),
		descHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 11, true)
	{
		
	}
};
class MotionBlur
{
private:
	uint vbuffer;
	uint tile2;
	uint tile4;
	uint tile8;
	uint tile;
	uint neighborMax;
	uint maxBlurPixels;
	uint tileSize;
	const uint sampleCount = 16;
	const float kMaxBlurRadius = 5.0f;
	const float shutterAngle = 270.0f;
	Shader* motionBlurShader;
public:
	void Init(std::vector<TemporalResourceCommand>& tempRT)
	{
		vbuffer = tempRT.size();
		RenderTextureFormat format;
		format.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
		format.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
		TemporalResourceCommand vbufferCommand;
		vbufferCommand.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
		vbufferCommand.uID = ShaderID::PropertyToID("_VelocityTex");
		vbufferCommand.descriptor.type = ResourceDescriptor::ResourceType_RenderTexture;
		vbufferCommand.descriptor.rtDesc.depthSlice = 1;
		vbufferCommand.descriptor.rtDesc.rtFormat = format;
		vbufferCommand.descriptor.rtDesc.type = RenderTextureDimension_Tex2D;
		tempRT.push_back(
			vbufferCommand
		);
		motionBlurShader = ShaderCompiler::GetShader("MotionBlur");
		TemporalResourceCommand tileCommand;
		format.colorFormat = DXGI_FORMAT_R16G16_SNORM;
		tileCommand.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
		tileCommand.uID = ShaderID::PropertyToID("_Tile2RT");
		tileCommand.descriptor.type = ResourceDescriptor::ResourceType_RenderTexture;
		tileCommand.descriptor.rtDesc.depthSlice = 1;
		tileCommand.descriptor.rtDesc.rtFormat = format;
		tileCommand.descriptor.rtDesc.type = RenderTextureDimension_Tex2D;
		tile2 = tempRT.size();
		tempRT.push_back(tileCommand);
		tileCommand.uID = ShaderID::PropertyToID("_Tile4RT");
		tile4 = tempRT.size();
		tempRT.push_back(tileCommand);
		tileCommand.uID = ShaderID::PropertyToID("_Tile8RT");
		tile8 = tempRT.size();
		tempRT.push_back(tileCommand);
		tileCommand.uID = ShaderID::PropertyToID("_TileVRT");
		tile = tempRT.size();
		tempRT.push_back(tileCommand);
		tileCommand.uID = ShaderID::PropertyToID("_NeighborMaxTex");
		neighborMax = tempRT.size();
		tempRT.push_back(tileCommand);
	}
	void UpdateTempRT(
		std::vector<TemporalResourceCommand>& tempRT,
		PipelineComponent::EventData& evt)
	{
		TemporalResourceCommand& vb = tempRT[vbuffer];
		vb.descriptor.rtDesc.width = evt.width;
		vb.descriptor.rtDesc.height = evt.height;
		TemporalResourceCommand& t2 = tempRT[tile2];
		t2.descriptor.rtDesc.width = evt.width / 2;
		t2.descriptor.rtDesc.height = evt.height / 2;
		TemporalResourceCommand& t4 = tempRT[tile4];
		t4.descriptor.rtDesc.width = evt.width / 4;
		t4.descriptor.rtDesc.height = evt.height / 4;
		TemporalResourceCommand& t8 = tempRT[tile8];
		t8.descriptor.rtDesc.width = evt.width / 8;
		t8.descriptor.rtDesc.height = evt.height / 8;
		maxBlurPixels = (uint)(kMaxBlurRadius * evt.height / 100);
		tileSize = ((maxBlurPixels - 1) / 8 + 1) * 8;
		TemporalResourceCommand& t = tempRT[tile];
		t.descriptor.rtDesc.width = evt.width / tileSize;
		t.descriptor.rtDesc.height = evt.height / tileSize;
		TemporalResourceCommand& neigh = tempRT[neighborMax];
		neigh.descriptor.rtDesc.width = evt.width / tileSize;
		neigh.descriptor.rtDesc.height = evt.height / tileSize;
	}

	void Execute(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		Camera* cam,
		FrameResource* res,
		RenderTexture* source,
		RenderTexture* dest,
		float4 _ZBufferParams,
		uint width, uint height)
	{
		MotionBlurCBufferData cbData;
		cbData._VelocityScale = shutterAngle / 360.0f;
		cbData._MaxBlurRadius = maxBlurPixels;
		cbData._RcpMaxBlurRadius = 1.0f / maxBlurPixels;
		float tileMaxOffs = (tileSize / 8.0f - 1.0f) * -0.5f;
		cbData._TileMaxOffs = tileMaxOffs;
		cbData._TileMaxLoop = floor(tileSize / 8.0f);
		cbData._LoopCount = max(sampleCount / 2, 1);
		cbData._ScreenParams = float2(width, height);
		cbData._ZBufferParams = _ZBufferParams;
		float4 mainTexSize(1.0f / width, 1.0f / height, width, height);
		cbData._VelocityTex_TexelSize = mainTexSize;
		uint tileWidth, tileHeight;
		tileWidth = width / tileSize;
		tileHeight = height / tileSize;
		cbData._NeighborMaxTex_TexelSize = float4(1.0f / tileWidth, 1.0f / tileHeight, tileWidth, tileHeight);
		MotionBlurFrameData* frameData = (MotionBlurFrameData*)res->GetPerCameraResource(this, cam, [&]()->MotionBlurFrameData* {
			return new MotionBlurFrameData(device);
		});
		frameData->constParams.CopyData(0, &cbData);
		uint currentWidth = width;
		uint currentHeight = height;
		cbData._MainTex_TexelSize = mainTexSize;
		frameData->constParams.CopyData(1, &cbData);
		currentWidth /= 2;
		currentHeight /= 2;
		cbData._MainTex_TexelSize = float4(1.0f / currentWidth, 1.0f / currentHeight, currentWidth, currentHeight);
		frameData->constParams.CopyData(2, &cbData);
		currentWidth /= 2;
		currentHeight /= 2;
		cbData._MainTex_TexelSize = float4(1.0f / currentWidth, 1.0f / currentHeight, currentWidth, currentHeight);
		frameData->constParams.CopyData(3, &cbData);
		currentWidth /= 2;
		currentHeight /= 2;
		cbData._MainTex_TexelSize = float4(1.0f / currentWidth, 1.0f / currentHeight, currentWidth, currentHeight);
		frameData->constParams.CopyData(4, &cbData);
		uint neighborMaxWidth = width / tileSize;
		uint neighborMaxHeight = height / tileSize;
		cbData._MainTex_TexelSize = float4(1.0f / neighborMaxWidth, 1.0f / neighborMaxHeight, neighborMaxWidth, neighborMaxHeight);
		frameData->constParams.CopyData(5, &cbData);
		cbData._MainTex_TexelSize = mainTexSize;
		frameData->constParams.CopyData(6, &cbData);
		motionBlurShader->BindRootSignature(commandList);
		//TODO
		//Bind DescriptorHeap
		//Graphics Blit
	}
};