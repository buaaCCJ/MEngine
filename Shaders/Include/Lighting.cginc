#ifndef __VOXELLIGHT_INCLUDE__
#define __VOXELLIGHT_INCLUDE__

#define XRES 32
#define YRES 16
#define ZRES 64
#define VOXELZ 64
#define MAXLIGHTPERCLUSTER 128
#define FROXELRATE 1.35
#define CLUSTERRATE 1.5
#define VOXELSIZE uint3(XRES, YRES, ZRES)

struct LightCommand{
    float3 direction;
	int shadowmapIndex;
	//
	float3 lightColor;
	uint lightType;
	//Align
	float3 position;
	float spotAngle;
	//Align
	float shadowSoftValue;
	float shadowBias;
	float shadowNormalBias;
	float range;
	//Align
	float spotRadius;
	float3 __align;
};

struct ReflectionProbe
{
	float3 position;
	float3 minExtent;
	float3 maxExtent;
	uint cubemapIndex;
};

uint GetIndex(uint3 id, uint3 size, uint multiply){
	uint3 multiValue = uint3(1, size.x, size.x * size.y) * multiply;
    return (uint)dot(id, multiValue);
}

#ifdef GBUFFER_SHADER
float3 CalculateLocalLight(
	float2 uv, 
	float4 WorldPos, 
	float linearDepth, 
	float3 WorldNormal,
	float3 ViewDir, 
	float cameraNear, 
	float lightFar,
	StructuredBuffer<uint> lightIndexBuffer,
	StructuredBuffer<LightCommand> lightBuffer)
{
	float ShadowTrem = 0;
	float3 ShadingColor = 0;
	float rate = pow(max(0, (linearDepth - cameraNear) /(lightFar - cameraNear)), 1.0 / CLUSTERRATE);
	rate = max(0, rate);
    if(rate > 1) return 0;
	uint3 voxelValue = uint3((uint2)(uv * float2(XRES, YRES)), (uint)(rate * ZRES));
	uint sb = GetIndex(voxelValue, VOXELSIZE, (MAXLIGHTPERCLUSTER + 1));
	uint c;

	float2 JitterSpot = uv;
	uint2 LightIndex = uint2(sb + 1, lightIndexBuffer[sb]);	// = uint2(sb + 1, _PointLightIndexBuffer[sb]);
	
	
	[loop]
	for (c = LightIndex.x; c < LightIndex.y; c++)
	{
		LightCommand lightCommand = lightBuffer[lightIndexBuffer[c]];
		ShadingColor += lightCommand.lightColor;
	}
	return ShadingColor;
}

float3 CalculateSunLight_NoShadow(float3 N, float3 V, float3 L, float3 col, float3 AlbedoColor, float3 SpecularColor, float3 Roughness, inout BSDFContext LightData)
{
	float3 H = normalize(V + L);
	InitGeoData(LightData, N, V);
	InitLightingData(LightData, N, V, L, H);
	return Defult_Lit(LightData, col,  AlbedoColor, SpecularColor, Roughness);

}
#endif

#endif