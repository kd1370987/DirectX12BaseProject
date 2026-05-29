#include "RaytracingEngine.h"

#include "Engine/Raytracing/RaytracingWorld/RaytracingWorld.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"

#include "../../Graphics/RenderContext/RenderContext.h"
#include "../../Graphics/GraphicEngine.h"
#include "../../D3D12/CBAllocater/CBAllocater.h"

#include "../RayPSO/RayPSO.h"
#include "../ShaderTable/ShaderTable.h"
namespace Engine::Raytracing
{



	void Engine::Raytracing::RayEngine::Commit()
	{
		m_upRayWorld->Commit();
	}

	void RayEngine::BindCamera(Graphics::RenderContext* a_pRCT, const Graphics::CameraData& a_cbCam)
	{
		auto* _pCmdList = a_pRCT->GetCurrentCmdList();
		// 定数バッファをバインド
		const auto& _cam = a_cbCam;

		m_camera.pos = { _cam.pos.x, _cam.pos.y, _cam.pos.z};
		m_camera.view = _cam.viewMat;
		m_camera.proj = _cam.projMat;
		m_camera.invView = _cam.viewInvMat;
		m_camera.invProj = _cam.projInvMat;
		m_camera.invViewProj = _cam.invViewProjMat;

		a_pRCT->BindCB()->BindAndAttachDataComputeRootCBV<Camera>(
			_pCmdList->NGet(),
			0,
			m_camera
		);
	}

	void RayEngine::BindTLAS(Graphics::RenderContext* a_pRCT)
	{
		auto* _pCmdList = a_pRCT->GetCurrentCmdList();

		// TLASをバインド
		_pCmdList->SetComputeRootShaderResourceView(1, m_upRayWorld->GetTLAS());
	}

	void RayEngine::Dispatch(Graphics::RenderContext* a_pRCT,ShaderTable& a_shadertable)
	{
		auto* _pCmdList = a_pRCT->GetCurrentCmdList();

		// 構造体バッファセット
		auto _gpuI = a_pRCT->GetGPUHandleBindLess(m_upRayWorld->GetInstanceBufferSRV());
		_pCmdList->SetComputeRootDescriptorTable(3, _gpuI);

		// マテリアル送信
		auto _gpuM = a_pRCT->GetGPUHandleBindLess(m_upRayWorld->GetMaterialBufferSRV());
		_pCmdList->SetComputeRootDescriptorTable(4, _gpuM);

		// シェーダーテーブル
		const auto& _desc = a_shadertable.GetDispatchDesc();
		_pCmdList->DispatchRays(&_desc);
	}

	void Engine::Raytracing::RayEngine::RegistModel(const DirectX::XMFLOAT4X4& a_worldMat, const Engine::Resource::Handle<Resource::Model>& a_modelHandle)
	{
		if (!m_upRayWorld)
		{
			m_upRayWorld = std::make_unique<RayWorld>();
		}

		// モデル登録
		m_upRayWorld->Register(a_worldMat, a_modelHandle);

		m_isCommit = false;
	}


	void Engine::Raytracing::RayEngine::CommitWorld()
	{
		// レイワールドの作成
		if (!m_upRayWorld)
		{
			m_upRayWorld = std::make_unique<RayWorld>();
		}
		// ヒットグループ数がいるが仮置き
		m_upRayWorld->Init(2);
	}

	void Engine::Raytracing::RayEngine::BegineFrame()
	{}

	void Engine::Raytracing::RayEngine::EndFrame()
	{
		m_upRayWorld->Clear();
	}

	const std::vector<Instance>& Engine::Raytracing::RayEngine::GetInstanceVec()
	{
		return m_upRayWorld->GetInstnace();
	}

	Engine::Raytracing::RayEngine::RayEngine()
	{}

	Engine::Raytracing::RayEngine::~RayEngine()
	{}
}