TextureCube _MainTex : register(t0, space0);
SamplerState gsamLinear  : register(s3);
cbuffer SkyboxBuffer : register(b0)
{
    float4x4 invVP;
    float deltaTime;
    float time;
};


struct appdata
{
	float3 vertex    : POSITION;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float3 worldView : TEXCOORD0;
};

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 0, 1);
    float4 worldPos = mul(invVP, float4(v.vertex.xy, 1, 1));
    worldPos.xyz /= worldPos.w;
    o.worldView = worldPos.xyz;
    return o;
}

float4 frag(v2f i) : SV_TARGET
{
    float mip = frac(time * 0.2);
    
    return _MainTex.SampleLevel(gsamLinear, i.worldView, mip * 10);
}