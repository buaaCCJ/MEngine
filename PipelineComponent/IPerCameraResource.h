#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
class Camera;
class PipelineComponent;
class FrameResource;
class IPipelineResource
{
public:
	virtual ~IPipelineResource() {}
};