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

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}

float4 frag(v2f i) : SV_TARGET
{
    float4 color = _MainTex[i.position.xy];
    return color;
}