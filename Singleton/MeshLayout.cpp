#include "MeshLayout.h"
std::unordered_map<USHORT, UINT> MeshLayout::layoutDict;
std::vector<std::vector<D3D12_INPUT_ELEMENT_DESC>*> MeshLayout::layoutValues;

std::vector<D3D12_INPUT_ELEMENT_DESC>* MeshLayout::GetMeshLayoutValue(UINT index)
{
	return layoutValues[index];
}

void MeshLayout::GenerateDesc(
	std::vector<D3D12_INPUT_ELEMENT_DESC>& target,
	bool normal,
	bool tangent,
	bool color,
	bool uv0,
	bool uv2,
	bool uv3,
	bool uv4,
	bool bone
)
{
	UINT offset = 12;
	target.reserve(8);
	target.push_back(
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	);
	if (normal)
	{
		target.push_back(
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 12;
	}
	if (tangent)
	{
		target.push_back(
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 16;
	}
	if (color)
	{
		target.push_back(
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 16;
	}
	if (uv0)
	{
		target.push_back(
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 8;
	}
	if (uv2)
	{
		target.push_back(
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 8;
	}
	if (uv3)
	{
		target.push_back(
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 8;
	}
	if (uv4)
	{
		target.push_back(
			{ "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 8;
	}
	if (bone)
	{
		target.push_back(
			{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		target.push_back(
			{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset + 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		);
		offset += 32;
	}
}
#include <mutex>
std::mutex mtx;
UINT MeshLayout::GetMeshLayoutIndex(
	bool normal,
	bool tangent,
	bool color,
	bool uv0,
	bool uv2,
	bool uv3,
	bool uv4,
	bool bone
)
{
	USHORT value = 0;
	if (normal) value |= 0b1;
	if (tangent) value |= 0b10;
	if (color) value |= 0b100;
	if (uv0) value |= 0b1000;
	if (uv2) value |= 0b10000;
	if (uv3) value |= 0b100000;
	if (uv4) value |= 0b1000000;
	if (bone) value |= 0b10000000;
	std::lock_guard<std::mutex> lck(mtx);
	auto&& ite = layoutDict.find(value);
	if (ite == layoutDict.end())
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC>* desc = new std::vector<D3D12_INPUT_ELEMENT_DESC>();
		GenerateDesc(*desc, normal, tangent, color, uv0, uv2, uv3, uv4, bone);
		layoutDict[value] = layoutValues.size();
		UINT value = (UINT)layoutValues.size();
		layoutValues.push_back(desc);
		return value;
	}
	return ite->second;
}