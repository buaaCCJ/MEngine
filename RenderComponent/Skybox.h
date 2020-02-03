#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class PSOContainer;
class FrameResource;
class ConstBufferElement;
class Shader;
class Mesh;
class Texture;
class Skybox : public MObject
{
private:
	static std::unique_ptr<Mesh> fullScreenMesh;
	ObjectPtr<Texture> skyboxTex;
	Shader* shader;
	UINT SkyboxCBufferID;
	UINT descHeapIndex;
public:
	virtual ~Skybox();
	Skybox(
		ObjectPtr<Texture>& tex,
		ID3D12Device* device
	);

	void Draw(
		int targetPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		FrameResource* currentResource,
		PSOContainer* container, uint containerIndex
	);
};
