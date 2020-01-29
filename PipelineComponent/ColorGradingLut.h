#pragma once
#include "../RenderComponent/RenderTexture.h"
#include "../Common/d3dUtil.h"
#include "../RenderComponent/Shader.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/UploadBuffer.h"
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

	float4 _CustomToneCurve;

	// Packing is currently borked, can't pass float arrays without it creating one vector4 per
	// float so we'll pack manually...
	float4 _ToeSegmentA;
	float4 _ToeSegmentB;
	float4 _MidSegmentA;
	float4 _MidSegmentB;
	float4 _ShoSegmentA;
	float4 _ShoSegmentB;
};
class ColorGradingLut
{
public:
	std::unique_ptr<RenderTexture> lut = nullptr;
	UploadBuffer cbuffer;
	DescriptorHeap heap;
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

	}
};