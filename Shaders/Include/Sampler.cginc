#ifndef SAMPLER_INCLUDE
#define SAMPLER_INCLUDE
SamplerState pointWrapSampler  : register(s0);
SamplerState pointClampSampler  : register(s1);
SamplerState bilinearWrapSampler  : register(s2);
SamplerState bilinearClampSampler  : register(s3);
SamplerState trilinearWrapSampler  : register(s4);
SamplerState trilinearClampSampler  : register(s5);
SamplerState anisotropicWrapSampler  : register(s6);
SamplerState anisotropicClampSampler  : register(s7);
SamplerComparisonState linearShadowSampler : register(s8);      //tex.SampleCmpLevelZero(linearShadowSampler, uv, testZ)

#endif