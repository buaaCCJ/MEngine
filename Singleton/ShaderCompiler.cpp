#include "ShaderCompiler.h"
#include "../RenderComponent/Shader.h"
#include "../RenderComponent/ComputeShader.h"
#include "../JobSystem/JobInclude.h"
namespace SCompile {
	struct TextureCube
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_DescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture2D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_DescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture3D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_DescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture2DArray
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_DescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};
	struct StructuredBuffer
	{
		inline static ShaderVariable GetShaderVar(UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_StructuredBuffer, 0, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::StructuredBuffer, 0, regis, space);
		}
	};

	struct ConstantBuffer
	{
		inline static ShaderVariable GetShaderVar(UINT regis, UINT space, const std::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_ConstantBuffer, 0, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::ConstantBuffer, 0, regis, space);
		}
	};


	struct RWTexture2D
	{
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWTexture3D
	{
		inline	static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWTexture2DArray
	{
		inline	static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWStructuredBuffer
	{
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const std::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::RWStructuredBuffer, 0, regis, space);
		}
	};
}
using namespace SCompile;
#define StructuredBuffer SCompile::StructuredBuffer
int ShaderCompiler::shaderIDCount = 0;

std::unordered_map<Shader*, int> ShaderCompiler::gbufferShaderIDs;
std::unordered_map<std::string, Shader*> ShaderCompiler::mShaders;
std::unordered_map<std::string, ComputeShader*> ShaderCompiler::mComputeShaders;
void ShaderCompiler::AddShader(std::string str, Shader* shad)
{
	mShaders[str] = shad;
}

void ShaderCompiler::AddComputeShader(std::string str, ComputeShader* shad)
{
	mComputeShaders[str] = shad;
}

Shader* ShaderCompiler::GetShader(std::string name)
{
	return mShaders[name];
}

ComputeShader* ShaderCompiler::GetComputeShader(std::string name)
{
	return mComputeShaders[name];
}

void GetPostProcessShader(ID3D12Device* device, JobBucket* bucket)
{
	//ZWrite
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	//Cull
	D3D12_RASTERIZER_DESC cullDesc;
	cullDesc.FillMode = D3D12_FILL_MODE_SOLID;
	cullDesc.CullMode = D3D12_CULL_MODE_NONE;
	cullDesc.FrontCounterClockwise = FALSE;
	cullDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	cullDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	cullDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	cullDesc.DepthClipEnable = FALSE;
	cullDesc.MultisampleEnable = FALSE;
	cullDesc.AntialiasedLineEnable = FALSE;
	cullDesc.ForcedSampleCount = 0;
	cullDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\PostProcess.hlsl";
	p.name = "PostProcess";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	const UINT SHADER_VAR_COUNT = 1;
	//Properties

	ShaderVariable var[SHADER_VAR_COUNT] =
	{
		Texture2D::GetShaderVar(1, 0, 0, "_MainTex")
	};
	Shader* opaqueShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("PostProcess", opaqueShader);
}

void GetOpaqueStandardShader(ID3D12Device* device, JobBucket* bucket)
{
	D3D12_DEPTH_STENCIL_DESC gbufferDsDesc;
	gbufferDsDesc.DepthEnable = TRUE;
	gbufferDsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	gbufferDsDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	gbufferDsDesc.StencilEnable = FALSE;
	gbufferDsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	gbufferDsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	D3D12_DEPTH_STENCIL_DESC depthPrepassDesc;
	depthPrepassDesc.DepthEnable = TRUE;
	depthPrepassDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthPrepassDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	depthPrepassDesc.StencilEnable = FALSE;
	depthPrepassDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthPrepassDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	gbufferDsDesc.FrontFace = defaultStencilOp;
	gbufferDsDesc.BackFace = defaultStencilOp;

	const UINT PASS_COUNT = 2;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "PS";
	p.vertex = "VS";
	p.filePath = L"Shaders\\Default.hlsl";
	p.name = "OpaqueStandard";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = gbufferDsDesc;
	Pass& dp = allPasses[1];
	dp.fragment = "PS_Depth";
	dp.vertex = "VS_Depth";
	dp.filePath = L"Shaders\\Default.hlsl";
	dp.name = "OpaqueStandard_Depth";
	dp.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	dp.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	dp.depthStencilState = depthPrepassDesc;
	const UINT SHADER_VAR_COUNT = 2;
	ShaderVariable var[SHADER_VAR_COUNT]
	{
		ConstantBuffer::GetShaderVar(0, 0, "Per_Object_Buffer"),
		ConstantBuffer::GetShaderVar(1, 0,"Per_Camera_Buffer")
	};
	Shader* opaqueShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("OpaqueStandard", opaqueShader);
}

void GetSkyboxShader(ID3D12Device* device, JobBucket* bucket)
{
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\Skybox.hlsl";
	p.name = "Skybox";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	const UINT SHADER_VAR_COUNT = 2;
	ShaderVariable var[SHADER_VAR_COUNT] =
	{
		ConstantBuffer::GetShaderVar(0, 0,"SkyboxBuffer"),
		TextureCube::GetShaderVar(1, 0, 0, "_MainTex"),
	};
	Shader* skyboxShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("Skybox", skyboxShader);
}

void GetGBufferRenderingShader(ID3D12Device* device, JobBucket* bucket)
{
	//ZWrite
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	//Cull
	D3D12_RASTERIZER_DESC cullDesc;
	cullDesc.FillMode = D3D12_FILL_MODE_SOLID;
	cullDesc.CullMode = D3D12_CULL_MODE_NONE;
	cullDesc.FrontCounterClockwise = FALSE;
	cullDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	cullDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	cullDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	cullDesc.DepthClipEnable = FALSE;
	cullDesc.MultisampleEnable = FALSE;
	cullDesc.AntialiasedLineEnable = FALSE;
	cullDesc.ForcedSampleCount = 0;
	cullDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\GBuffer.hlsl";
	p.name = "GBuffer";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;

	const UINT SHADER_VAR_COUNT = 9;
	ShaderVariable var[SHADER_VAR_COUNT] =
	{
		Texture2D::GetShaderVar(6, 0, 0, "_MainTex"),
		Texture2D::GetShaderVar(6, 0, 1, "_GreyTex"),
		Texture2D::GetShaderVar(6, 0, 2, "_IntegerTex"),
		Texture2D::GetShaderVar(6, 0, 3, "_Cubemap"),
		ConstantBuffer::GetShaderVar(0, 0, "Per_Camera_Buffer"),
		StructuredBuffer::GetShaderVar(0, 4,"_AllLight"),
		StructuredBuffer::GetShaderVar(1,4,"_LightIndexBuffer"),
		ConstantBuffer::GetShaderVar(1, 0,"LightCullCBuffer"),
		ConstantBuffer::GetShaderVar(2, 0, "TextureIndices")
	};

	Shader* gbufferShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("GBuffer", gbufferShader);
}

void GetTemporalAAShader(ID3D12Device* device, JobBucket* bucket)
{
	//ZWrite
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	//Cull
	D3D12_RASTERIZER_DESC cullDesc;
	cullDesc.FillMode = D3D12_FILL_MODE_SOLID;
	cullDesc.CullMode = D3D12_CULL_MODE_NONE;
	cullDesc.FrontCounterClockwise = FALSE;
	cullDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	cullDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	cullDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	cullDesc.DepthClipEnable = FALSE;
	cullDesc.MultisampleEnable = FALSE;
	cullDesc.AntialiasedLineEnable = FALSE;
	cullDesc.ForcedSampleCount = 0;
	cullDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\TemporalAA.hlsl";
	p.name = "TemporalAA";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	const UINT SHADER_VAR_COUNT = 2;
	ShaderVariable vars[2] =
	{
		ConstantBuffer::GetShaderVar(0, 0, "TAAConstBuffer"),
		Texture2D::GetShaderVar(6,0,0, "_MainTex")
	};

	Shader* taaShader = new Shader(allPasses, PASS_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("TemporalAA", taaShader);
}

void GetCullingShader(ID3D12Device* device, JobBucket* bucket)
{
	const UINT KERNEL_COUNT = 2;
	std::string kernelNames[KERNEL_COUNT];
	kernelNames[0] = "CSMain";
	kernelNames[1] = "Clear";
	const UINT SHADER_VAR_COUNT = 5;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		ConstantBuffer::GetComputeVar(0,0, "CullBuffer"),
		StructuredBuffer::GetComputeVar(0, 0,"_InputBuffer"),
		RWStructuredBuffer::GetComputeVar(0, 0, "_OutputBuffer"),
		RWStructuredBuffer::GetComputeVar(1, 0, "_CountBuffer"),
		StructuredBuffer::GetComputeVar(1, 0, "_InputDataBuffer")
	};
	ComputeShader* cs = new ComputeShader(L"Shaders\\Cull.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("Cull", cs);
}

void GetClusterBasedLightCullingShader(ID3D12Device* device, JobBucket* bucket)
{
	const UINT KERNEL_COUNT = 3;
	std::string kernelNames[KERNEL_COUNT];
	kernelNames[0] = "SetXYPlane";
	kernelNames[1] = "SetZPlane";
	kernelNames[2] = "CBDR";
	const UINT SHADER_VAR_COUNT = 4;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		ConstantBuffer::GetComputeVar(0, 0, "LightCullCBuffer"),
		RWTexture2D::GetComputeVar(2, 0, 0, "_MainTex"),
		RWStructuredBuffer::GetComputeVar(0, 1,"_LightIndexBuffer"),
		StructuredBuffer::GetComputeVar(0, 0, "_AllLight")
	};
	ComputeShader* cs = new ComputeShader(L"Shaders\\LightCull.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("LightCull", cs);
}

void ShaderCompiler::Init(ID3D12Device* device, JobSystem* jobSys)
{
#ifdef NDEBUG
#define SHADER_MULTICORE_COMPILE	
#endif
	JobBucket* bucket = nullptr;
#ifdef SHADER_MULTICORE_COMPILE
	bucket = jobSys->GetJobBucket();
#endif
	mShaders.reserve(50);
	mComputeShaders.reserve(50);
	GetOpaqueStandardShader(device, bucket);
	GetSkyboxShader(device, bucket);
	GetCullingShader(device, bucket);
	GetPostProcessShader(device, bucket);
	GetTemporalAAShader(device, bucket);
	GetClusterBasedLightCullingShader(device, bucket);
	GetGBufferRenderingShader(device, bucket);
#ifdef SHADER_MULTICORE_COMPILE
	jobSys->ExecuteBucket(bucket, 1);
#endif
}

void ShaderCompiler::Dispose()
{
	mComputeShaders.clear();
	mShaders.clear();
}