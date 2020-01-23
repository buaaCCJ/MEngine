#define PI 3.1415926536
uint ReverseBits32(uint bits)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
	return bits;
}

float2 Hammersley(uint Index, uint NumSamples, uint2 Random)
{
	float E1 = frac((float)Index / NumSamples + float(Random.x & 0xffff) / (1 << 16));
	float E2 = float(ReverseBits32(Index) ^ Random.y) * 2.3283064365386963e-10;
	return float2(E1, E2);
}

uint HaltonSequence(uint Index, uint base = 3)
{
	uint result = 0;
	uint f = 1;
	uint i = Index;
	
	[loop] 
	while (i > 0) {
		result += (f / base) * (i % base);
		i = floor(i / base);
	}
	return result;
}

float4 ImportanceSampleGGX(float2 E, float Roughness) {
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m2 - 1) * E.y) );
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 H = float3( SinTheta * cos(Phi), SinTheta * sin(Phi), CosTheta );
			
	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (PI * d * d);
			
	float PDF = D * CosTheta;
	return float4(H, PDF);
}

float IBL_PBR_Specular_G(float NoL, float NoV, float a) {
    float a2 = a * a;
    a2 *= a2;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return (2 * NoL) / (GGXV + GGXL);
}


float2 IBL_Defualt_SpecularIntegrated(float Roughness, float NoV) {
    float3 V;
    V.x = sqrt(1 - NoV * NoV);
    V.y = 0;
    V.z = NoV;

    float2 r = 0;
	const uint NumSamples = 1024;
    [loop]
    for (uint i = 0; i < NumSamples; i++) {
        float2 E = Hammersley(i, NumSamples, HaltonSequence(i)); 
        float4 H = ImportanceSampleGGX(E, Roughness);
        float3 L = 2 * dot(V, H) * H.xyz - V;

        float VoH = saturate(dot(V, H.xyz));
        float NoL = saturate(L.z);
        float NoH = saturate(H.z);

        if (NoL > 0) {
            float G = IBL_PBR_Specular_G(NoL, NoV, Roughness);
            float Gv = G * VoH / NoH;
            float Fc = pow(1 - VoH, 5);
            //r.x += Gv * (1 - Fc);
            r.x += Gv;
            r.y += Gv * Fc;
        }
    }
    return r / NumSamples;
}

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

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}

float2 frag(v2f i) : SV_TARGET
{
    return IBL_Defualt_SpecularIntegrated(i.uv.x, i.uv.y);
}