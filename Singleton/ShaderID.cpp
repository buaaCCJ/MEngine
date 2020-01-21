#include "ShaderID.h"
std::unordered_map<std::string, unsigned int> ShaderID::allShaderIDs;
unsigned int ShaderID::currentCount = 0;
unsigned int ShaderID::mPerCameraBuffer = 0;
unsigned int ShaderID::mPerMaterialBuffer = 0;
unsigned int ShaderID::mPerObjectBuffer = 0;
unsigned int ShaderID::mainTex = 0;
std::mutex ShaderID::mtx;
unsigned int ShaderID::PropertyToID(std::string str)
{
	mtx.lock();
	auto&& ite = allShaderIDs.find(str);
	if (ite == allShaderIDs.end())
	{
		unsigned int value = currentCount;
		allShaderIDs[str] = currentCount;
		++currentCount;
		mtx.unlock();
		return value;
	}
	else
	{
		mtx.unlock();
		return ite->second;
	}
	
}

void ShaderID::Init()
{
	allShaderIDs.reserve(INIT_CAPACITY);
	mPerCameraBuffer = PropertyToID("Per_Camera_Buffer");
	mPerMaterialBuffer = PropertyToID("Per_Material_Buffer");
	mPerObjectBuffer = PropertyToID("Per_Object_Buffer");
	mainTex = PropertyToID("_MainTex");
}