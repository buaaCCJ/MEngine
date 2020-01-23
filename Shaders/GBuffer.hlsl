	#include "Include/Sampler.cginc"
	#include "Include/Common.hlsl"
	#include "Include/Montcalo_Library.hlsl"
	#include "Include/BSDF_Library.hlsl"
	#include "Include/ShadingModel.hlsl"
	#define GBUFFER_SHADER
	#include "Include/Lighting.cginc"
	struct appdata
		{
			float4 vertex : POSITION;
			float2 texcoord : TEXCOORD;
		};

		struct v2f
		{
			float4 vertex : SV_POSITION;
			float2 texcoord : TEXCOORD;
		};

		Texture2D<float4> _MainTex[6] : register(t0, space0);
		Texture2D<float> _GreyTex[6] : register(t0, space1);
		Texture2D<uint> _IntegerTex[6] : register(t0, space2);
		TextureCube<float4> _Cubemap[6] : register(t0, space3);

cbuffer Per_Camera_Buffer : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 nonJitterVP;
    float4x4 nonJitterInverseVP;
    float4x4 lastVP;
    float4x4 inverseLastVP;
    float3 worldSpaceCameraPos;
    float gNearZ;
    float gFarZ;
};

cbuffer LightCullCBuffer : register(b1)
{
    float4x4 _InvVP;
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
};

cbuffer TextureIndices : register(b2)
{
	uint _UVTex;
	uint _TangentTex;
    uint _NormalTex ;
    uint _DepthTex ;
    uint _ShaderIDTex ;
    uint _MaterialIDTex;
	uint _SkyboxTex;
};

inline float LinearEyeDepth( float z )
{
    return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}

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

StructuredBuffer<LightCommand> _AllLight : register(t0, space4);
StructuredBuffer<uint> _LightIndexBuffer : register(t1, space4);

		v2f vert(appdata v)
		{
			v2f o;
			o.vertex = float4(v.vertex.xy, 0, 1);
			o.texcoord = v.texcoord;
			return o;
		}

        float4 frag(v2f i) : SV_TARGET
        {
			float depth = _GreyTex[_DepthTex][i.vertex.xy];
			float4 uv = _MainTex[_UVTex][i.vertex.xy];
			float4 tangent = _MainTex[_TangentTex][i.vertex.xy] * 2 - 1;
			float3 worldNormal = normalize(_MainTex[_NormalTex][i.vertex.xy].xyz * 2 - 1);
			float4 worldPos = mul(_InvVP, float4((i.texcoord * 2 - 1) * float2(1, -1), depth, 1));
			worldPos /= worldPos.w;
			float3 viewDir = normalize(worldSpaceCameraPos - worldPos.xyz);
			uint shaderID = _IntegerTex[_ShaderIDTex][i.vertex.xy];
			uint matID = _IntegerTex[_MaterialIDTex][i.vertex.xy];
			float linearEyeDepth = LinearEyeDepth(depth);

			float3 refl = reflect(-viewDir, worldNormal);
			float4 skyboxColor = _Cubemap[_SkyboxTex].SampleLevel(trilinearClampSampler, refl, 3);
		/*	float3 lightColor = CalculateLocalLight(
				i.texcoord,
				worldPos,
				linearEyeDepth,
				worldNormal,
				viewDir,
				_CameraNearPos.w,
				_CameraFarPos.w,
				_LightIndexBuffer,
				_AllLight
			);*/
		
			//CalculateSunLight_NoShadow(float3 N, float3 V, float3 L, float3 col, float3 AlbedoColor, float3 SpecularColor, float3 Roughness)
			float3 sunColor = CalculateSunLight_NoShadow(
				worldNormal,
				viewDir,
				-_SunDir,
				_SunColor,
				0,
				1,
				0.7
			);
			return float4(sunColor, 1);//float4(worldNormal, 1);
        }