#include "../../Common/d3dUtil.h"
#pragma once
const int DistortionIntensity = 5;
const float DistortionCenterX = 0;
const float DistortionCenterY = 0;
const float DistortionIntensityX = 1;
const float DistortionIntensityY = 1;
const float DistortionScale = 1;
const float ChromaticAberrationIntensity = 0.15f;
void GetLensDistortion(
	float4& centerScale,
	float4& amountResult,
	float& chromaticAberrationInten
) {
	
	float amount = abs(DistortionIntensity);
	amount = 1.6f * max<float>(amount, 1);
	float theta = 0.0174532924 * min<float>(160, amount);
	float sigma = 2.0 * tan(theta * 0.5);
	Vector4 p0(DistortionCenterX, DistortionCenterY, max<float>(DistortionIntensityX, 1e-4), max<float>(DistortionIntensityY, 1e-4));
	Vector4 p1(DistortionIntensity >= 0 ? theta : 1.0 / theta, sigma, 1.0 / DistortionScale, DistortionIntensity);
	centerScale = p0;
	amountResult = p1;
	chromaticAberrationInten = ChromaticAberrationIntensity * 0.05f;
}