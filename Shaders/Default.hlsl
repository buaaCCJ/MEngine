cbuffer Per_Object_Buffer : register(b0)
{
    float4x4 _LocalToWorld;
    uint2 _ID;   //shaderid, materialid
};

cbuffer Per_Camera_Buffer : register(b1)
{
    float4x4 _WorldToCamera;
    float4x4 _InverseWorldToCamera;
    float4x4 _Proj;
    float4x4 _InvProj;
    float4x4 _VP;
    float4x4 _InvVP;
    float4x4 _NonJitterVP;
    float4x4 _NonJitterInverseVP;
    float4x4 _LastVP;
    float4x4 _InverseLastVP;
    float3 worldSpaceCameraPos;
    float _NearZ;
    float _FarZ;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 uv    : TEXCOORD;
    float4 tangent : TANGENT;
    float2 uv2     : TEXCOORD1;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float4 tangent : TANGENT;
	float4 uv      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(_LocalToWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul((float3x3)_LocalToWorld, vin.NormalL);

    // Transform to homogeneous clip space.
    vout.PosH = mul(_VP, posW);
    //vout.PosH /= vout.PosH.w;
    //vout.PosH.z = 1 - vout.PosH.z;
    vout.uv.xy = float4(vin.uv.xy, vin.uv2.xy);

    vout.tangent.xyz = mul((float3x3)_LocalToWorld, vin.tangent.xyz);
    vout.tangent.w = vin.tangent.w;
    return vout;
}

void PS(VertexOut i, 
        out float4 tangentTex : SV_TARGET0,
        out float4 uvTex : SV_TARGET1,
        out float4 normalTex : SV_TARGET2,
        out float2 motionVectorTex : SV_TARGET3,
        out uint shaderIDTex : SV_TARGET4,
        out uint materialIDTex : SV_TARGET5)
{
   uvTex = frac(i.uv);
   tangentTex = i.tangent * 0.5 + 0.5;
   normalTex = float4(i.NormalW * 0.5 + 0.5, 1);
   motionVectorTex = 0;
   shaderIDTex = _ID.x;
   materialIDTex = _ID.y;
   
}

float4 VS_Depth(float3 position : POSITION) : SV_POSITION
{
    float4 posW = mul(_LocalToWorld, float4(position, 1));
    return mul(_VP, posW);
}

void PS_Depth(){}