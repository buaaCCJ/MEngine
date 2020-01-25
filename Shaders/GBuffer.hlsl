
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


			return float4(sunColor + preint, 1);//float4(worldNormal, 1);
        }