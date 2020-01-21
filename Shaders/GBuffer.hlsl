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

        Texture2D _MainTex[6] : register(t0, space0);   //Sign
		
		Texture2D<float4> _ColorTex[6] : register(t0, space0);
		Texture2D<float> _GreyTex[6] : register(t0, space0);
		Texture2D<uint> _IntegerTex[6] : register(t0, space0);

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
};

cbuffer TextureIndices : register(b2)
{
	uint _UVTex;
	uint _TangentTex;
    uint _NormalTex ;
    uint _DepthTex ;
    uint _ShaderIDTex ;
    uint _MaterialIDTex;

};

inline float LinearEyeDepth( float z )
{
    return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}


StructuredBuffer<LightCommand> _AllLight : register(t0, space1);
StructuredBuffer<uint> _LightIndexBuffer : register(t1, space1);

		v2f vert(appdata v)
		{
			v2f o;
			o.vertex = float4(v.vertex.xy, 0, 1);
			o.texcoord = v.texcoord;
			return o;
		}

        float4 frag(v2f i) : SV_TARGET
        {
			float4 uv = _ColorTex[_UVTex][i.vertex.xy];
			float4 tangent = _ColorTex[_TangentTex][i.vertex.xy] * 2 - 1;
			float3 normal = normalize(_ColorTex[_NormalTex][i.vertex.xy].xyz * 2 - 1);
			float depth = _GreyTex[_DepthTex][i.vertex.xy];
			uint shaderID = _IntegerTex[_ShaderIDTex][i.vertex.xy];
			uint matID = _IntegerTex[_MaterialIDTex][i.vertex.xy];
			float linearEyeDepth = LinearEyeDepth(depth);
			float4 worldPos = mul(gInvViewProj, float4((i.texcoord * 2 - 1) * float2(1, -1), depth, 1));
			worldPos /= worldPos.w;
			float3 viewDir = normalize(worldSpaceCameraPos - worldPos.xyz);
			float3 lightColor = CalculateLocalLight(
				i.texcoord,
				worldPos,
				linearEyeDepth,
				normal,
				viewDir,
				_CameraNearPos.w,
				_CameraFarPos.w,
				_LightIndexBuffer,
				_AllLight
			);

			return float4(normal, 1);
        }