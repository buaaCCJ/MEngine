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

SamplerState pointWrapSampler  : register(s0);
SamplerState pointClampSampler  : register(s1);
SamplerState linearWrapSampler  : register(s2);
SamplerState linearClampSampler  : register(s3);
SamplerState anisotropicWrapSampler  : register(s4);
SamplerState anisotropicClampSampler  : register(s5);
v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}

float4 frag(v2f i) : SV_TARGET
{
    float4 color = _MainTex.SampleLevel(pointClampSampler, i.uv, 0);
    //color.xyz = dot(color.xyz, 0.3333333);
    return color;
}