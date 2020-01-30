#pragma once
#include "../RenderComponent/RenderTexture.h"
#include "../Common/d3dUtil.h"
#include "../RenderComponent/Shader.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../Singleton/ColorUtility.h"
#include "../RenderComponent/ComputeShader.h"
using namespace Math;
struct ColorGradingCBuffer
{
	float4 _Size; // x: lut_size, y: 1 / (lut_size - 1), zw: unused

	float4 _ColorBalance;
	float4 _ColorFilter;
	float4 _HueSatCon;

	float4 _ChannelMixerRed;
	float4 _ChannelMixerGreen;
	float4 _ChannelMixerBlue;

	float4 _Lift;
	float4 _InvGamma;
	float4 _Gain;
};
class ColorGradingLut
{
public:
	const int k_Lut2DSize = 32;
	const int k_Lut3DSize = 128;
	std::unique_ptr<RenderTexture> lut = nullptr;
	UploadBuffer cbuffer;
	DescriptorHeap heap;
	ColorGradingLut(ID3D12Device* device) :
		heap(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			1,
			true),
		cbuffer(
			device, 1,
			true, sizeof(ColorGradingCBuffer))
	{
	}
	const float temperature = 0.0f;
	const float tint = 0.0f;
	const Vector4 filterColor = { 1,1,1,1 };
	const float hueShift = 0;
	const float saturation = 0;
	const float contrast = 0;
	const float mixerRedOutRedIn = 100;
	const float mixerRedOutGreenIn = 0;
	const float mixerRedOutBlueIn = 0;
	const float mixerGreenOutRedIn = 0;
	const float mixerGreenOutGreenIn = 100;
	const float mixerGreenOutBlueIn = 0;
	const float mixerBlueOutRedIn = 0;
	const float mixerBlueOutGreenIn = 0;
	const float mixerBlueOutBlueIn = 100;
	const Vector4 lift = { 1.0f,1.0f,1.0f,0.0f};
	const Vector4 gamma = { 1.0f,1.0f,1.0f,0.0f};
	const Vector4 gain = { 1.0f,1.0f,1.0f,0.0f};

	void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (lut) return;
		RenderTextureFormat format;
		format.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
		format.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		lut = std::unique_ptr<RenderTexture>(new RenderTexture(
			device,
			128, 128,
			format,
			RenderTextureDimension_Tex3D,
			128, 1, RenderTextureState::Unordered_Access
		));
		ComputeShader* lutBakeShader = ShaderCompiler::GetComputeShader("Lut3DBaker");
		ColorGradingCBuffer& cb = *(ColorGradingCBuffer*)cbuffer.GetMappedDataPtr(0);
		cb._Size = { (float)k_Lut3DSize, 1.0f / (k_Lut3DSize - 1.0f), 0.0f, 0.0f};
		Vector3 colorBalance = ColorUtility::ComputeColorBalance(temperature, tint);
		cb._ColorBalance = colorBalance;
		cb._ColorFilter = (Vector4)filterColor;
		float hue = hueShift / 360.0f;         // Remap to [-0.5;0.5]
		float sat = saturation / 100.0f + 1.0f;  // Remap to [0;2]
		float con = contrast / 100.0f + 1.0f;    // Remap to [0;2]
		cb._HueSatCon = { hue, sat, con, 0.0f };
		Vector4 channelMixerR(mixerRedOutRedIn, mixerRedOutGreenIn, mixerRedOutBlueIn, 0.0f);
		Vector4 channelMixerG(mixerGreenOutRedIn, mixerGreenOutGreenIn, mixerGreenOutBlueIn, 0.0f);
		Vector4 channelMixerB(mixerBlueOutRedIn, mixerBlueOutGreenIn, mixerBlueOutBlueIn, 0.0f);
		cb._ChannelMixerRed = channelMixerR / 100.0f;
		cb._ChannelMixerGreen = channelMixerG / 100.0f;
		cb._ChannelMixerBlue = channelMixerB / 100.0f;
		Vector4 lift = ColorUtility::ColorToLift(lift * 0.2f);
		Vector4 gain = ColorUtility::ColorToGain(gain * 0.8f);
		Vector4 invgamma = ColorUtility::ColorToInverseGamma(gamma * 0.8f);
		cb._InvGamma = invgamma;
		cb._Lift = lift;
		cb._Gain = gain;
		uint groupSize = k_Lut3DSize / 4;
		lut->BindUAVToHeap(&heap, 0, device, 0);
		lutBakeShader->BindRootSignature(cmdList, &heap);
		lutBakeShader->SetResource(cmdList, ShaderID::PropertyToID("Params"), &cbuffer, 0);
		lutBakeShader->SetResource(cmdList, ShaderID::PropertyToID("_MainTex"), &heap, 0);
		lutBakeShader->Dispatch(cmdList, 0, groupSize, groupSize, groupSize);
	}
};