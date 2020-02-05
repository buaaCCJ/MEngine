#pragma once
#include "../IPerCameraResource.h"
#include "../../Common/d3dUtil.h"
class RenderTexture;
struct CameraTransformData : public IPipelineResource
{
	DirectX::XMFLOAT2 jitter;
	DirectX::XMFLOAT2 lastFrameJitter;
	DirectX::XMMATRIX nonJitteredVPMatrix;
	DirectX::XMMATRIX nonJitteredProjMatrix;
	DirectX::XMMATRIX lastNonJitterVP;
};