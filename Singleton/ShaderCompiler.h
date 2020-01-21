#pragma once
#include "../Common/d3dUtil.h"
class JobSystem;
class Shader;
class ComputeShader;
class ID3D12Device;
class MaterialManager;
class ShaderCompiler
{
private:
	static int shaderIDCount;
	static std::unordered_map<std::string, ComputeShader*> mComputeShaders;
	static std::unordered_map<std::string, Shader*> mShaders;
	static std::unordered_map<Shader*, int> gbufferShaderIDs;
public:
	static Shader* GetShader(std::string name);
	static ComputeShader* GetComputeShader(std::string name);
	static void AddShader(std::string str, Shader* shad);
	static void AddComputeShader(std::string str, ComputeShader* shad);
	static void Init(ID3D12Device* device, JobSystem* jobSys);
	static void Dispose();
};