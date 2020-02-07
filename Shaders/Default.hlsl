	#include "Include/Sampler.cginc"
	#include "Include/Common.hlsl"
	#include "Include/Montcalo_Library.hlsl"
	#include "Include/BSDF_Library.hlsl"
	#include "Include/ShadingModel.hlsl"
	#define GBUFFER_SHADER
	#include "Include/Lighting.cginc"

		Texture2D<float4> _MainTex[6] : register(t0, space0);
		Texture2D<float> _GreyTex[6] : register(t0, space1);
		Texture2D<uint> _IntegerTex[6] : register(t0, space2);
		TextureCube<float4> _Cubemap[6] : register(t0, space3);

cbuffer Per_Object_Buffer : register(b0)
{
    float4x4 _LastLocalToWorld;
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

cbuffer LightCullCBuffer : register(b2)
{
    float4x4 _inverseVP;
	float4 _CameraNearPos;
	float4 _CameraFarPos;
	float4 _ZBufferParams;
	float3 _CameraForward;
	uint _LightCount;
	float3 _SunColor;
	uint _SunEnabled;
	float3 _SunDir;
	uint _SunShadowEnabled;
	uint4 _ShadowmapIndices;
	float4 _CascadeDistance;
	float4x4 _ShadowMatrix[4];
	uint _ReflectionProbeCount;
};

cbuffer TextureIndices : register(b3)
{
	uint _SkyboxTex;
	uint _PreintTexture;
};

cbuffer ProjectionShadowParams : register(b4)
{
	float4x4 _ShadowmapVP;
	float3 _LightPos;
};

StructuredBuffer<LightCommand> _AllLight : register(t0, space4);
StructuredBuffer<uint> _LightIndexBuffer : register(t1, space4);

struct StandardPBRMaterial
{
	float2 uvScale;
	float2 uvOffset;
	//align
	float3 albedo;
	float metallic;
	float3 emission;
	float roughness;
	//align
	float occlusion;
	float cutoff;
	int albedoTexIndex;
	int specularTexIndex;
	//align
	int normalTexIndex;
	int emissionTexIndex;
	float2 __align;
};

StructuredBuffer<StandardPBRMaterial> _DefaultMaterials : register(t2, space4);
inline float LinearEyeDepth( float z )
{
    return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}

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
    float4 lastProjPos : TEXCOORD1;
    float4 currentProjPos : TEXCOORD2;
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
    vout.currentProjPos = mul(_NonJitterVP, posW);
    vout.lastProjPos = mul(_LastVP, mul(_LastLocalToWorld, float4(vin.PosL, 1.0f)));
    //vout.PosH /= vout.PosH.w;
    //vout.PosH.z = 1 - vout.PosH.z;
    vout.uv.xy = float4(vin.uv.xy, vin.uv2.xy);

    vout.tangent.xyz = mul((float3x3)_LocalToWorld, vin.tangent.xyz);
    vout.tangent.w = vin.tangent.w;
    return vout;
}

void PS(VertexOut i, 
        out float4 albedo : SV_TARGET0,
        out float4 specular : SV_TARGET1,
        out float4 normalTex : SV_TARGET2,
        out float2 motionVectorTex : SV_TARGET3,
        out float4 emissionRT : SV_TARGET4)
{
   float2 lastScreenUV = (i.lastProjPos.xy / i.lastProjPos.w) * float2(0.5, 0.5) + 0.5;
   float2 screenUV = (i.currentProjPos.xy / i.currentProjPos.w) * float2(0.5, 0.5) + 0.5;
   motionVectorTex = screenUV - lastScreenUV;
   float3 viewDir = normalize(worldSpaceCameraPos - i.PosW.xyz);
   i.NormalW = normalize(i.NormalW);
    float linearEyeDepth = LinearEyeDepth(i.PosH.z);
    float3 refl = reflect(-viewDir, i.NormalW);
		/*	float3 lightColor = CalculateLocalLight(
				i.texcoord,
				i.PosW.xyz,
				linearEyeDepth,
				worldNormal,
				viewDir,
				_CameraNearPos.w,
				_CameraFarPos.w,
				_LightIndexBuffer,
				_AllLight
			);*/
		
			//CalculateSunLight_NoShadow(float3 N, float3 V, float3 L, float3 col, float3 AlbedoColor, float3 SpecularColor, float3 Roughness)
			BSDFContext context = (BSDFContext)0;
			float3 sunColor = CalculateSunLight_NoShadow(
				i.NormalW,
				viewDir,
				-_SunDir,
				_SunColor,
				0.8,
				0.2,
				1,
				context
			);
			float4 skyboxColor = _Cubemap[_SkyboxTex].SampleLevel(trilinearClampSampler, refl, 10);
			float2 preintAB = _MainTex[_PreintTexture].SampleLevel(bilinearClampSampler, float2(1, context.NoV), 0).rg;
			float3 EnergyCompensation;
			float3 preint = PreintegratedDGF_LUT(preintAB, EnergyCompensation, skyboxColor.xyz);
			preint *= EnergyCompensation;

   albedo = 0;
   specular = 0;
   normalTex = float4(i.NormalW * 0.5 + 0.5, 1);
   emissionRT = float4(sunColor + preint, 1);
}

float4 VS_Depth(float3 position : POSITION) : SV_POSITION
{
    float4 posW = mul(_LocalToWorld, float4(position, 1));
    return mul(_VP, posW);
}

void PS_Depth(){}

float4 VS_Shadowmap(float3 position : POSITION) : SV_POSITION
{
	float4 posW = mul(_LocalToWorld, float4(position, 1));
	return mul(_ShadowmapVP, posW);
}

void PS_Shadowmap(){}