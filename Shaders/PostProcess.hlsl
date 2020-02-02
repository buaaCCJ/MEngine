#include "Include/D3D12.hlsl"
#include "Include/Sampler.cginc"
#include "Include/StdLib.cginc"
#include "Include/ACES.hlsl"
#include "Include/Colors.hlsl"
#include "Include/Distortion.cginc"
struct appdata
{
	float3 vertex    : POSITION;
    float2 uv : TEXCOORD;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float2 uv : TEXCOORD;
};
Texture2D<float4> _MainTex : register(t0, space0);
Texture3D<float4> _Lut3D : register(t1, space0);
cbuffer Params : register(b0)
{
    float4 _Lut3DParam;
    float4 _Distortion_Amount;
    float4 _Distortion_CenterScale;
    float4 _MainTex_TexelSize;
    float _ChromaticAberration_Amount;
};
#define MAX_CHROMATIC_SAMPLES 16
v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}


float4 frag(v2f i) : SV_TARGET
{
    float2 uv = i.uv;
    bool weight = dot(abs(uv - 0.5) , 0.5) <= dot(_MainTex_TexelSize.xy, 0.5);
    float2 distortedUV = DistortCheck(uv, weight, _Distortion_CenterScale, _Distortion_Amount);
   // float4 color = _MainTex.SampleLevel(bilinearClampSampler, distortedUV, 0);
   //Chromatic
   float2 coords = 2.0 * uv - 1.0;
			float2 end = uv - coords * dot(coords, coords) * _ChromaticAberration_Amount;

			float2 diff = end - uv;
			int samples = clamp(int(length(_MainTex_TexelSize.zw * diff / 2.0)), 3, MAX_CHROMATIC_SAMPLES);
			float2 delta = diff / samples;
			float2 pos = uv;
			float4 sum = (0.0).xxxx, filterSum = (0.0).xxxx;

			for (int i = 0; i < samples; i++)
			{
				float t = (i + 0.5) / samples;
				float4 s = _MainTex.SampleLevel(bilinearClampSampler, DistortCheck(pos,weight, _Distortion_CenterScale, _Distortion_Amount), 0);
				float3 filterDiff = abs(float3(
                    t,
                    t - 0.5,
                    t - 1
                ));
                float4 filter = float4(1 - saturate(filterDiff * 2), 1);
				sum += s * filter;
				filterSum += filter;
				pos += delta;
			}

	float4 color = sum / filterSum;
    //Color Grading
    color *= 1.5;
    float3 colorLutSpace = saturate(LinearToLogC(color.rgb));
	color.rgb = ApplyLut3D(_Lut3D, bilinearClampSampler, colorLutSpace, _Lut3DParam.xy);

    return color;
}