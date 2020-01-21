#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/Pool.h"
class MeshRenderer;
struct RendererNode
{
	DirectX::XMFLOAT3 boundingCenter;
	DirectX::XMFLOAT3 boundingExtent;
	DirectX::XMMATRIX localToWorldMatrix;
	DirectX::XMFLOAT3 worldBoundingCenter;
	DirectX::XMFLOAT3 worldBoundingExtent;
	MeshRenderer* rendererPtr;
};

template <typename T, UINT mipCount>
class CubeTree
{
private:
	T* arrs[mipCount];
	UINT sizes[mipCount];
public:
	CubeTree()
	{
		UINT size = 1;
		for (UINT i = 0; i < mipCount; ++i)
		{
			UINT curSize = size * size * size;
			arrs[i] = new T[curSize];
			memset(arrs[i], 0, curSize * sizeof(T));
			sizes[i] = size;
			size *= 2;
		}
	}
	~CubeTree()
	{
		for (UINT i = 0; i < mipCount; ++i)
			delete[] arrs[i];
	}

	T& Get(UINT mip, UINT x, UINT y, UINT z)
	{
		UINT currentSize = sizes[mip];
#ifdef NDEBUG	//Release
		return arrs[mip][z * currentSize * currentSize + y * currentSize + x];
#else	//Debug
		if (x >= currentSize || y >= currentSize || z >= currentSize || mip >= mipCount)
		{
			T* resultPtr = nullptr;
			return *resultPtr;
		}
		return arrs[mip][z * currentSize * currentSize + y * currentSize + x];
#endif
	}
};

struct SceneTreeNode
{
	SceneTreeNode* next;
	SceneTreeNode* last;
	RendererNode node;
	SceneTreeNode(const RendererNode& node) :
		node(node), next(nullptr), last(nullptr) {}
	void RemoveThis(Pool<SceneTreeNode>& pool);
	void AddAfter(const RendererNode& node, Pool<SceneTreeNode>& pool);
};

class SceneTree
{
public:
	static const UINT maxTreeLevel = 6;
private:

	CubeTree<SceneTreeNode*, maxTreeLevel> tree;
	DirectX::XMVECTOR minSide;
	DirectX::XMVECTOR maxSide;
	float fullSize;
public:
	SceneTree(
		DirectX::XMVECTOR center,
		DirectX::XMVECTOR extent);
	void AddMeshRenderer(MeshRenderer* targetRenderer, DirectX::XMMATRIX&& currentLocalToWorld, DirectX::XMVECTOR&& center, DirectX::XMVECTOR&& extent, Pool<SceneTreeNode>& pool);
	void TransformMeshRenderer(MeshRenderer* targetRenderer, DirectX::XMMATRIX&& lastLocalToWorld, DirectX::XMMATRIX&& currentLocalToWorld, DirectX::XMVECTOR&& center, DirectX::XMVECTOR&& extent);
	void RemoveMeshRenderer(MeshRenderer* targetRenderer, DirectX::XMMATRIX&& currentLocalToWorld, DirectX::XMVECTOR&& center, DirectX::XMVECTOR&& extent, Pool<SceneTreeNode>& pool);
	~SceneTree();
};