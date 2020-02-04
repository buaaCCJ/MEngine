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
		descHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 9, true)
	{

	}
};
class MotionBlur final
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
	uint _CameraDepthTexture;
	uint _CameraMotionVectorsTexture;
	uint _NeighborMaxTex;
	uint _VelocityTex;
	uint Params;
	Storage<PSOContainer, 1> containerStorage;
	PSOContainer* container;
public:
	void Init(std::vector<TemporalResourceCommand>& tempRT)
	{
		_CameraDepthTexture = ShaderID::PropertyToID("_CameraDepthTexture");
		_CameraMotionVectorsTexture = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
		_NeighborMaxTex = ShaderID::PropertyToID("_NeighborMaxTex");
		_VelocityTex = ShaderID::PropertyToID("_VelocityTex");
		Params = ShaderID::PropertyToID("Params");
		container = (PSOContainer*)&containerStorage;
		PSORTSetting settings[3];
		settings[0].depthFormat = DXGI_FORMAT_UNKNOWN;
		settings[0].rtCount = 1;
		settings[0].rtFormat[0] = DXGI_FORMAT_R10G10B10A2_UNORM;

		settings[1].depthFormat = DXGI_FORMAT_UNKNOWN;
		settings[1].rtCount = 1;
		settings[1].rtFormat[0] = DXGI_FORMAT_R16G16_FLOAT;

		settings[2].depthFormat = DXGI_FORMAT_UNKNOWN;
		settings[2].rtCount = 1;
		settings[2].rtFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		new (container) PSOContainer(settings, _countof(settings));
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
		vbufferCommand.descriptor.rtDesc.state = RenderTextureState::Render_Target;
		tempRT.push_back(
			vbufferCommand
		);
		motionBlurShader = ShaderCompiler::GetShader("MotionBlur");
		TemporalResourceCommand tileCommand;
		format.colorFormat = DXGI_FORMAT_R16G16_FLOAT;
		tileCommand.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
		tileCommand.uID = ShaderID::PropertyToID("_Tile2RT");
		tileCommand.descriptor.type = ResourceDescriptor::ResourceType_RenderTexture;
		tileCommand.descriptor.rtDesc.depthSlice = 1;
		tileCommand.descriptor.rtDesc.rtFormat = format;
		tileCommand.descriptor.rtDesc.type = RenderTextureDimension_Tex2D;
		tileCommand.descriptor.rtDesc.state = RenderTextureState::Render_Target;
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
		ThreadCommand* tCmd,
		Camera* cam,
		FrameResource* res,
		RenderTexture* source,
		RenderTexture* dest,
		RenderTexture** tempRT,
		float4 _ZBufferParams,
		uint width, uint height, uint depthTexture, uint mvTexture)
	{
		auto commandList = tCmd->GetCmdList();
		MotionBlurCBufferData cbData;
		cbData._VelocityScale = shutterAngle / 360.0f;
		cbData._MaxBlurRadius = maxBlurPixels;
		cbData._RcpMaxBlurRadius = 1.0f / maxBlurPixels;
		float tileMaxOffs = (tileSize / 8.0f - 1.0f) * -0.5f;
		cbData._TileMaxOffs = tileMaxOffs;
		cbData._TileMaxLoop = floor(tileSize / 8.0f);
		cbData._LoopCount = max<uint>(sampleCount / 2, 1);
		cbData._ScreenParams = float2(width, height);
		cbData._ZBufferParams = _ZBufferParams;
		float4 mainTexSize(1.0f / width, 1.0f / height, width, height);
		cbData._VelocityTex_TexelSize = mainTexSize;
		uint tileWidth, tileHeight;
		tileWidth = width / tileSize;
		tileHeight = height / tileSize;
		cbData._NeighborMaxTex_TexelSize = float4(1.0f / tileWidth, 1.0f / tileHeight, tileWidth, tileHeight);
		cbData._MainTex_TexelSize = mainTexSize;
		MotionBlurFrameData* frameData = (MotionBlurFrameData*)res->GetPerCameraResource(this, cam, [&]()->MotionBlurFrameData* {
			return new MotionBlurFrameData(device);
		});
		frameData->constParams.CopyData(0, &cbData);
		uint currentWidth = width;
		uint currentHeight = height;
		
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
		tempRT[depthTexture]->BindColorBufferToSRVHeap(&frameData->descHeap, 0, device);
		tempRT[mvTexture]->BindColorBufferToSRVHeap(&frameData->descHeap, 1, device);
		tempRT[neighborMax]->BindColorBufferToSRVHeap(&frameData->descHeap, 2, device);
		tempRT[vbuffer]->BindColorBufferToSRVHeap(&frameData->descHeap, 3, device);
		tempRT[tile2]->BindColorBufferToSRVHeap(&frameData->descHeap, 4, device);
		tempRT[tile4]->BindColorBufferToSRVHeap(&frameData->descHeap, 5, device);
		tempRT[tile8]->BindColorBufferToSRVHeap(&frameData->descHeap, 6, device);
		tempRT[tile]->BindColorBufferToSRVHeap(&frameData->descHeap, 7, device);
		source->BindColorBufferToSRVHeap(&frameData->descHeap, 8, device);
		motionBlurShader->BindRootSignature(commandList, &frameData->descHeap);
		motionBlurShader->SetResource(commandList, _CameraDepthTexture, &frameData->descHeap, 0);
		motionBlurShader->SetResource(commandList, _CameraMotionVectorsTexture, &frameData->descHeap, 1);
		motionBlurShader->SetResource(commandList, _NeighborMaxTex, &frameData->descHeap, 2);
		motionBlurShader->SetResource(commandList, _VelocityTex, &frameData->descHeap, 3);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 0);
		Graphics::Blit(
			commandList, device,
			&tempRT[vbuffer]->GetColorDescriptor(0),
			1, nullptr,
			container, 0,
			tempRT[vbuffer]->GetWidth(),
			tempRT[vbuffer]->GetHeight(),
			motionBlurShader, 0);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 3);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 1);
		tCmd->SetResourceReadWriteState(tempRT[vbuffer], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&tempRT[tile2]->GetColorDescriptor(0),
			1, nullptr,
			container, 1,
			tempRT[tile2]->GetWidth(),
			tempRT[tile2]->GetHeight(),
			motionBlurShader, 1);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 4);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 2);
		tCmd->SetResourceReadWriteState(tempRT[tile2], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&tempRT[tile4]->GetColorDescriptor(0),
			1, nullptr,
			container, 1,
			tempRT[tile4]->GetWidth(),
			tempRT[tile4]->GetHeight(),
			motionBlurShader, 2);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 5);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 3);
		tCmd->SetResourceReadWriteState(tempRT[tile4], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&tempRT[tile8]->GetColorDescriptor(0),
			1, nullptr,
			container, 1,
			tempRT[tile8]->GetWidth(),
			tempRT[tile8]->GetHeight(),
			motionBlurShader, 2);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 6);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 4);
		tCmd->SetResourceReadWriteState(tempRT[tile8], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&tempRT[tile]->GetColorDescriptor(0),
			1, nullptr,
			container, 1,
			tempRT[tile]->GetWidth(),
			tempRT[tile]->GetHeight(),
			motionBlurShader, 3);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 7);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 5);
		tCmd->SetResourceReadWriteState(tempRT[tile], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&tempRT[neighborMax]->GetColorDescriptor(0),
			1, nullptr,
			container, 1,
			tempRT[neighborMax]->GetWidth(),
			tempRT[neighborMax]->GetHeight(),
			motionBlurShader, 4);
		motionBlurShader->SetResource(commandList, ShaderID::GetMainTex(), &frameData->descHeap, 8);
		motionBlurShader->SetResource(commandList, Params, &frameData->constParams, 6);
		tCmd->SetResourceReadWriteState(tempRT[neighborMax], ResourceReadWriteState::Read);
		Graphics::Blit(
			commandList, device,
			&dest->GetColorDescriptor(0),
			1, nullptr,
			container, 2,
			dest->GetWidth(),
			dest->GetHeight(),
			motionBlurShader, 5);
	}
	~MotionBlur()
	{
		container->~PSOContainer();
	}
};