#pragma once
#include <unordered_map>
#include <mutex>
class ShaderID
{
	static const unsigned int INIT_CAPACITY = 100;
	static unsigned int currentCount;
	static std::unordered_map<std::string, unsigned int> allShaderIDs;
	static unsigned int mPerCameraBuffer;
	static unsigned int mPerMaterialBuffer;
	static unsigned int mPerObjectBuffer;
	static unsigned int mainTex;
	static std::mutex mtx;
public:
	static void Init();
	static unsigned int GetPerCameraBufferID() { return mPerCameraBuffer; }
	static unsigned int GetPerMaterialBufferID() { return mPerMaterialBuffer; }
	static unsigned int GetPerObjectBufferID() { return mPerObjectBuffer; }
	static unsigned int GetMainTex() { return mainTex; }
	static unsigned int PropertyToID(std::string str);

};

