#pragma once
#include "../RenderComponent/Shader.h"
#include "../RenderComponent/ComputeShader.h"
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