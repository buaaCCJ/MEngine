#ifndef __SHADING_MODEL__
#define __SHADING_MODEL__

float3 Defult_Lit(BSDFContext LightData, float3 Energy,  float3 AlbedoColor, float3 SpecularColor, float Roughness)
{
    float3 Diffuse = Diffuse_RenormalizeBurley(LightData.LoH, LightData.NoL, LightData.NoV, AlbedoColor, Roughness);

    float pbr_GGX = D_GGX(LightData.NoH, Roughness);     
    float pbr_Vis = Vis_SmithGGXCorrelated(LightData.NoL, LightData.NoV, Roughness); 
    float3 pbr_Fresnel = F_Schlick(SpecularColor, 1, LightData.VoH);     

    float3 Specular = max(0, pbr_Vis * pbr_GGX * pbr_Fresnel);
    return (Diffuse + Specular) * Energy* LightData.NoL;
}

float3 Skin_Lit(BSDFContext LightData, float3 Energy,  float3 AlbedoColor, float3 SpecularColor, float Roughness, float skinRoughness)
{
    float3 Diffuse = Diffuse_RenormalizeBurley(LightData.LoH, LightData.NoL, LightData.NoV, AlbedoColor, Roughness);

	float2 pbr_GGX = D_Beckmann(LightData.NoH, float2(Roughness, skinRoughness));
	pbr_GGX.x = lerp(pbr_GGX.x, pbr_GGX.y, 0.15);
	float pbr_Vis = Vis_SmithGGXCorrelated(LightData.NoL, LightData.NoV, Roughness);
	float3 pbr_Fresnel = F_Schlick(SpecularColor, 1, LightData.VoH);

	float3 Specular =  max(0, (pbr_Vis * pbr_GGX.x) * pbr_Fresnel );

	return (Diffuse + Specular) * Energy * LightData.NoL;
}

float3 ClearCoat_Lit(BSDFContext LightData, float3 Energy, float3 ClearCoat_MultiScatterEnergy, float3 AlbedoColor, float3 SpecularColor, float ClearCoat, float ClearCoat_Roughness, float Roughness)
{
	float3 Diffuse = Diffuse_RenormalizeBurley(LightData.LoH, LightData.NoL, LightData.NoV, AlbedoColor, Roughness);
	float F0 = pow5(1 - LightData.VoH);

	float ClearCoat_GGX = D_GGX(LightData.NoH, ClearCoat_Roughness);
	float ClearCoat_Vis = Vis_Kelemen(LightData.VoH);
	float ClearCoat_Fersnel = (F0 + (1 - F0) * 0.05) * ClearCoat;
	float3 ClearCoat_Specular = ClearCoat_GGX * ClearCoat_Vis * ClearCoat_Fersnel;
	ClearCoat_Specular *= ClearCoat_MultiScatterEnergy ;

    float pbr_GGX = D_GGX(LightData.NoH, Roughness);     
    float pbr_Vis = Vis_SmithGGXCorrelated(LightData.NoL, LightData.NoV, Roughness);
	float3 pbr_Fresnel = saturate(50 * SpecularColor.g) * F0 + (1 - F0) * SpecularColor;
	float3 BaseSpecular = (pbr_Vis * pbr_GGX) * pbr_Fresnel;


	float LayerAttenuation = (1 - ClearCoat_Fersnel);
	return (Diffuse + max(0, BaseSpecular) +  max(0, ClearCoat_Specular)) * Energy * LayerAttenuation * LightData.NoL;
}

float3 PreintegratedDGF_LUT(float2 AB, out float3 EnergyCompensation, float3 SpecularColor)
{
    float3 ReflectionGF = lerp(saturate(50 * SpecularColor.g) * AB.ggg, AB.rrr, SpecularColor);
    EnergyCompensation = 1 + SpecularColor * (1 / AB.r - 1);
    return ReflectionGF;
}

/*
#if CLEARCOAT_LIT
struct GeometryBuffer
{
	float3 AlbedoColor;
	float3 SpecularColor;
	float Roughness;
	float3 ClearCoat_MultiScatterEnergy;
	float ClearCoat;
	float ClearCoat_Roughness;
};
#elif SKIN_LIT
struct GeometryBuffer
{
	float3 AlbedoColor;
	float3 SpecularColor;
	float Roughness;
	float Skin_Roughness;
};
#else
struct GeometryBuffer
{
	float3 AlbedoColor;
	float3 SpecularColor;
	float Roughness;
};
#endif
float3 LitFunc(BSDFContext LightData, float3 Energy, GeometryBuffer buffer)
{
#if SKIN_LIT
return Skin_Lit(LightData, Energy,  buffer.AlbedoColor, buffer.SpecularColor, buffer.Roughness, buffer.Skin_Roughness);
#elif CLOTH_LIT
return Cloth_Cotton(LightData, Energy, buffer.AlbedoColor, buffer.SpecularColor, buffer.Roughness);
#elif CLEARCOAT_LIT
return ClearCoat_Lit(LightData, Energy,  buffer.ClearCoat_MultiScatterEnergy, buffer.AlbedoColor, buffer.SpecularColor, buffer.ClearCoat, buffer.ClearCoat_Roughness, buffer.Roughness);
#else
return Defult_Lit(LightData, Energy,  buffer.AlbedoColor, buffer.SpecularColor, buffer.Roughness);
#endif
}
*/
#endif
