#include "SceneTree.h"
#include "../RenderComponent/MeshRenderer.h"
#include "../Common/MetaLib.h"
using namespace DirectX;
SceneTree::SceneTree(
	DirectX::XMVECTOR center,
	DirectX::XMVECTOR extent
) : minSide(center - extent), maxSide(center + extent)
{
	XMVECTOR fullSizeVec = XMVector3Length(extent);
	fullSize = ((float*)&fullSizeVec)[0] * 2;
}

void GenerateWorldBound(
	XMMATRIX&& localToWorldTransposed,
	XMVECTOR&& center,
	XMVECTOR&& extent,
	XMVECTOR& resultMin,
	XMVECTOR& resultMax
)
{
	XMVECTOR pos[8];
	XMVECTOR rate;
	pos[0] = XMVector3TransformCoord(center + extent, localToWorldTransposed);
	rate = { 1,1,-1,1 };
	pos[1] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	rate = { 1,-1,1,1 };
	pos[2] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	rate = { 1,-1,-1,1 };
	pos[3] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	rate = { -1,1,-1,1 };
	pos[4] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	rate = { -1,-1,1,1 };
	pos[5] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	rate = { -1,1,1,1 };
	pos[6] = XMVector3TransformCoord(center + extent * rate, localToWorldTransposed);
	pos[7] = XMVector3TransformCoord(center - extent, localToWorldTransposed);
	const float maxValue = 4294967296;
	const float minValue = -4294967296;
	XMFLOAT3 minVec(maxValue, maxValue, maxValue);
	XMFLOAT3 maxVec(minValue, minValue, minValue);
	auto func = [&](UINT i)->void
	{
		XMFLOAT3 currentWorldPos;
		XMStoreFloat3(&currentWorldPos, pos[i]);
		if (currentWorldPos.x < minVec.x)
		{
			minVec.x = currentWorldPos.x;
		}
		if (currentWorldPos.y < minVec.y)
		{
			minVec.y = currentWorldPos.y;
		}
		if (currentWorldPos.z < minVec.z)
		{
			minVec.z = currentWorldPos.z;
		}

		if (currentWorldPos.x > maxVec.x)
		{
			maxVec.x = currentWorldPos.x;
		}
		if (currentWorldPos.y > maxVec.y)
		{
			maxVec.y = currentWorldPos.y;
		}
		if (currentWorldPos.z > maxVec.z)
		{
			maxVec.z = currentWorldPos.z;
		}
	};
	InnerLoop<decltype(func), 8>(func);
	resultMin = XMLoadFloat3(&minVec);
	resultMax = XMLoadFloat3(&maxVec);
}

void GetCenterExtent(const XMVECTOR& minPos, const XMVECTOR& maxPos, XMVECTOR* resultCenter, XMVECTOR* resultExtent)
{
	XMVECTOR halfMinPos = minPos * 0.5;
	XMVECTOR halfMaxPos = maxPos * 0.5;
	*resultCenter = halfMinPos + halfMaxPos;
	*resultExtent = halfMaxPos - halfMinPos;
}

UINT GetCoveredSize(
	DirectX::XMMATRIX&& currentLocalToWorld,
	DirectX::XMVECTOR&& center,
	DirectX::XMVECTOR&& extent,
	float fullSize,
	XMVECTOR& minSide,
	UINT* minValues,
	UINT* maxValues,
	RendererNode* renderNode
)
{
	XMMATRIX matrixTranspose = XMMatrixTranspose(currentLocalToWorld);
	XMVECTOR worldMin;
	XMVECTOR worldMax;
	GenerateWorldBound(
		std::move(matrixTranspose),
		std::move(center),
		std::move(extent),
		worldMin,
		worldMax
	);
	//Get World Bounding
	XMVECTOR worldCenter, worldExtent;
	GetCenterExtent(worldMin, worldMax, &worldCenter, &worldExtent);
	//Generate Renderer Data
	memcpy(&renderNode->boundingCenter, &center, sizeof(XMFLOAT3));
	memcpy(&renderNode->boundingExtent, &extent, sizeof(XMFLOAT3));
	memcpy(&renderNode->worldBoundingCenter, &worldCenter, sizeof(XMFLOAT3));
	memcpy(&renderNode->worldBoundingExtent, &worldExtent, sizeof(XMFLOAT3));
	renderNode->localToWorldMatrix = currentLocalToWorld;
	//Get Mip Level
	XMVECTOR sizeLengthVec = XMVector3Length(worldMax - worldMin);
	float vecLength = *((float*)&sizeLengthVec);
	UINT level = (UINT)(0.8 + log2(fullSize / vecLength));
	const UINT legalLevel = SceneTree::maxTreeLevel - 1;
	level = min(level, legalLevel);
	level = max<uint>(level, 0);
	double size = pow(2.0, level);
	UINT sizeInt = (UINT)(size - 0.9);
	//Get UV
	auto GetIndex = [&](XMVECTOR& minSide, float size, XMVECTOR& vec)->XMVECTOR
	{
		return (vec - minSide) / size;
	};
	XMVECTOR minPointNormalizedPos = GetIndex(minSide, fullSize, worldMin);
	XMVECTOR maxPointNormalizedPos = GetIndex(minSide, fullSize, worldMax);
	XMVECTOR minPointIndex = minPointNormalizedPos * size;
	XMVECTOR maxPointIndex = maxPointNormalizedPos * size;
	UINT minX, minY, minZ;
	UINT maxX, maxY, maxZ;
	float* minPointIndexPtr = (float*)&minPointIndex;
	float* maxPointIndexPtr = (float*)&maxPointIndex;
	auto setMinFunc = [&](UINT i)->void
	{
		minValues[i] = min((UINT)minPointIndexPtr[i], sizeInt);
	};
	auto setMaxFunc = [&](UINT i)->void
	{
		maxValues[i] = min((UINT)maxPointIndexPtr[i], sizeInt);
	};
	InnerLoop<decltype(setMinFunc), 3>(setMinFunc);
	InnerLoop<decltype(setMaxFunc), 3>(setMaxFunc);
	return level;
}

void SceneTree::AddMeshRenderer(
	MeshRenderer* targetRenderer,
	DirectX::XMMATRIX&& currentLocalToWorld,
	DirectX::XMVECTOR&& center,
	DirectX::XMVECTOR&& extent,
	Pool<SceneTreeNode>& pool)
{
	UINT minValues[3];
	UINT maxValues[3];
	RendererNode renderNode;
	UINT level = GetCoveredSize(
		std::move(currentLocalToWorld),
		std::move(center),
		std::move(extent),
		fullSize,
		minSide,
		minValues,
		maxValues,
		&renderNode);
	renderNode.rendererPtr = targetRenderer;
	//Loop Set
	for (UINT x = minValues[0]; x <= maxValues[0]; ++x)
		for (UINT y = minValues[1]; y <= maxValues[1]; ++y)
			for (UINT z = minValues[2]; z <= maxValues[2]; ++z)
			{
				SceneTreeNode*& node = tree.Get(level, x, y, z);
				SceneTreeNode* newNode = pool.New<const RendererNode&>(renderNode);
				newNode->next = node;
				node = newNode;
			}
}
void SceneTree::TransformMeshRenderer(
	MeshRenderer* targetRenderer,
	DirectX::XMMATRIX&& lastLocalToWorld,
	DirectX::XMMATRIX&& currentLocalToWorld,
	DirectX::XMVECTOR&& center,
	DirectX::XMVECTOR&& extent)
{
	//TODO
}
void SceneTree::RemoveMeshRenderer(
	MeshRenderer* targetRenderer,
	DirectX::XMMATRIX&& currentLocalToWorld,
	DirectX::XMVECTOR&& center,
	DirectX::XMVECTOR&& extent,
	Pool<SceneTreeNode>& pool)
{
	UINT minValues[3];
	UINT maxValues[3];
	RendererNode renderNode;
	UINT level = GetCoveredSize(
		std::move(currentLocalToWorld),
		std::move(center),
		std::move(extent),
		fullSize,
		minSide,
		minValues,
		maxValues,
		&renderNode);
	//Loop Set
	for (UINT x = minValues[0]; x <= maxValues[0]; ++x)
		for (UINT y = minValues[1]; y <= maxValues[1]; ++y)
			for (UINT z = minValues[2]; z <= maxValues[2]; ++z)
			{
				SceneTreeNode* node = tree.Get(level, x, y, z);
				while (node != nullptr)
				{
					if (node->node.rendererPtr == targetRenderer)
					{
						node->RemoveThis(pool);
						return;
					}
					node = node->next;
				}
			}
}
SceneTree::~SceneTree()
{
	//SceneTreeNode* node = (SceneTreeNode*)&rootNode;
	//node->~SceneTreeNode();
}


void SceneTreeNode::RemoveThis(Pool<SceneTreeNode>& pool)
{
	if (last)
	{
		last->next = next;
	}
	if (next)
	{
		next->last = last;
	}
	pool.Delete(this);
}
void SceneTreeNode::AddAfter(const RendererNode& node, Pool<SceneTreeNode>& pool)
{
	SceneTreeNode* newNode = pool.New<const RendererNode&>(node);
	if (next)
	{
		next->last = newNode;
	}
	newNode->next = next;
	newNode->last = this;
	next = newNode;
}