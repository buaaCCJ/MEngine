//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include "RenderComponent/Shader.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "RenderComponent/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "Singleton/FrameResource.h"
#include "Singleton/ShaderID.h"
#include "RenderComponent/Texture.h"
#include "Singleton/MeshLayout.h"
#include "Singleton/PSOContainer.h"
#include "Common/Camera.h"
#include "RenderComponent/Mesh.h"
#include "RenderComponent/MeshRenderer.h"
#include "Singleton/ShaderCompiler.h"
#include "RenderComponent/Skybox.h"
#include "RenderComponent/ComputeShader.h"
#include "RenderComponent/RenderTexture.h"
#include "PipelineComponent/RenderPipeline.h"
#include "Singleton/Graphics.h"
#include "Singleton/MathLib.h"
#include "JobSystem/JobInclude.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

#include "LogicComponent/World.h"
class CrateApp : public D3DApp
{
public:
	std::vector<JobBucket*> buckets[2];
	bool bucketsFlag = false;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeCommandQueue;
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize()override;
	virtual void OnResize()override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	//void AnimateMaterials(const GameTimer& gt);
	//void UpdateObjectCBs(const GameTimer& gt);

	std::unique_ptr<JobSystem> pipelineJobSys;
	void BuildFrameResources();
	bool lastFrameExecute = false;
	int mCurrFrameResourceIndex = 0;
	RenderPipeline* rp;
	ObjectPtr<Camera> mainCamera;
	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;
	POINT mLastMousePos;
	FrameResource* lastResource = nullptr;
	std::unique_ptr<ThreadCommand> directThreadCommand;
};
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
	RenderPipeline::DestroyInstance();
	ShaderCompiler::Dispose();
	World::DestroyInstance();
	for (int i = 0; i < FrameResource::mFrameResources.size(); ++i)
	{
		FrameResource::mFrameResources[i] = nullptr;
	}
	pipelineJobSys = nullptr;
}

bool CrateApp::Initialize()
{

	if (!D3DApp::Initialize())
		return false;
	ShaderID::Init();
	buckets[0].reserve(20);
	buckets[1].reserve(20);
	UINT cpuCoreCount = std::thread::hardware_concurrency() - 2;	//One for main thread & one for loading
	pipelineJobSys = std::unique_ptr<JobSystem>(new JobSystem(max(1, cpuCoreCount)));
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeCommandQueue)));
	directThreadCommand = std::unique_ptr<ThreadCommand>(new ThreadCommand(md3dDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT));
	directThreadCommand->ResetCommand();
	// Reset the command list to prep for initialization commands.
	ShaderCompiler::Init(md3dDevice.Get(), pipelineJobSys.get());
	GeometryGenerator geoGen;
	//BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), boxMesh);
	//BuildShapeGeometry(geoGen.CreateSphere(1, 32, 33), sphereMesh);
	//BuildMaterials();
	BuildFrameResources();
	mainCamera = new Camera(md3dDevice.Get(), Camera::CameraRenderPath::DefaultPipeline);
	//BuildPSOs();
	// Execute the initialization commands.
	// Wait until initialization is complete.
	World::CreateInstance(directThreadCommand->GetCmdList(), md3dDevice.Get());
	rp = RenderPipeline::GetInstance(md3dDevice.Get(),
		directThreadCommand->GetCmdList());
	Graphics::Initialize(md3dDevice.Get(), directThreadCommand->GetCmdList());
	directThreadCommand->CloseCommand();
	ID3D12CommandList* lst = directThreadCommand->GetCmdList();
	mCommandQueue->ExecuteCommandLists(1, &lst);
	FlushCommandQueue();
	directThreadCommand = nullptr;


	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();
	lastFrameExecute = false;
}

std::vector<Camera*> cam(1);
void CrateApp::Draw(const GameTimer& gt)
{
	if (mClientHeight < 1 || mClientWidth < 1) return;
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	lastResource = FrameResource::mCurrFrameResource;
	FrameResource::mCurrFrameResource = FrameResource::mFrameResources[mCurrFrameResourceIndex].get();
//	FrameResource::mCurrFrameResource->UpdateBeforeFrame(mFence.Get());
	std::vector <JobBucket*>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;

	//Logic
	JobBucket* mainLogicBucket = pipelineJobSys->GetJobBucket();
	bucketArray.push_back(mainLogicBucket);
	JobHandle cameraUpdateJob = mainLogicBucket->GetTask([&]()->void
	{
		OnKeyboardInput(gt);
		UpdateCamera(gt);

		mainCamera->SetLens(0.333333 * MathHelper::Pi, AspectRatio(), 1.5, 100);
		World::GetInstance()->Update(FrameResource::mCurrFrameResource);

	});
	//Rendering
	cam[0] = mainCamera.operator->();
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	RenderPipelineData data;
	data.device = md3dDevice.Get();
	data.backBufferHandle = CurrentBackBufferView();
	data.backBufferResource = CurrentBackBuffer();
	data.lastResource = lastResource;
	data.resource = FrameResource::mCurrFrameResource;
	data.allCameras = &cam;
	data.fence = mFence.Get();
	data.fenceIndex = &mCurrentFence;
	data.executeLastFrame = lastFrameExecute;
	data.world = World::GetInstance();
	data.world->windowWidth = mClientWidth;
	data.world->windowHeight = mClientHeight;
	data.deltaTime = gt.DeltaTime();
	data.time = gt.TotalTime();
	rp->PrepareRendering(data, pipelineJobSys.get(), bucketArray);
	pipelineJobSys->Wait();
	data.resource->UpdateBeforeFrame(data.fence);//Flush CommandQueue
	pipelineJobSys->ExecuteBucket(bucketArray.data(), bucketArray.size());					//Execute Tasks
	rp->ExecuteRendering(data, bucketArray);
	ThrowIfFailed(mSwapChain->Present(0, 0));
	std::vector <JobBucket*>& lastBucketArray = buckets[bucketsFlag];
	for (auto ite = lastBucketArray.begin(); ite != lastBucketArray.end(); ++ite)
	{
		pipelineJobSys->ReleaseJobBucket(*ite);
	}
	lastBucketArray.clear();
	lastFrameExecute = true;
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
}

void CrateApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	XMFLOAT3 mEyePos;
	mEyePos.x = mRadius * sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	XMFLOAT3 target = { 0,0,0 };
	XMFLOAT3 up = { 0,1,0 };
	mainCamera->LookAt(mEyePos, target, up);

}

void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		FrameResource* res = new FrameResource(md3dDevice.Get(), 1, 1);
		FrameResource::mFrameResources.emplace_back(res);
		res->commandBuffer = std::unique_ptr<CommandBuffer>(new CommandBuffer(mCommandQueue.Get(), mComputeCommandQueue.Get()));
	}
}