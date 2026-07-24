#include "RaytracingEngine.h"

#include "Engine/Raytracing/RaytracingWorld/RaytracingWorld.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Graphics/RenderContext/RenderContext.h"
#include "../../Graphics/GraphicEngine.h"
#include "../../D3D12/CBAllocater/CBAllocater.h"

#include "../RayPSO/RayPSO.h"
#include "../ShaderTable/ShaderTable.h"
namespace Engine::Raytracing
{



	void RayEngine::Release()
	{
		// 未初期化(レイトレ未使用)でも安全に呼べるようにする
		if (m_upRayWorld)
		{
			m_upRayWorld->Release();
			m_upRayWorld.reset();
		}
	}

	void Engine::Raytracing::RayEngine::Commit(D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (m_isCommit)  return;
		m_upRayWorld->Commit(a_pCmdList);
		m_isCommit = true;
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

	void Engine::Raytracing::RayEngine::RegistModel(
		const DirectX::XMFLOAT4X4& a_worldMat,
		const Engine::Handle<Resource::Model>& a_modelHandle,
		const DXSM::Vector4& a_colorScale,
		const DXSM::Vector3& a_emissiveScale
	)
	{
		if (!m_upRayWorld)
		{
			m_upRayWorld = std::make_unique<RayWorld>();
		}

		// モデル登録
		m_upRayWorld->Register(a_worldMat, a_modelHandle,a_colorScale,a_emissiveScale);

		m_isCommit = false;
	}

	void RayEngine::RegisterSkinningModel(ECS::World& a_world, const DXSM::Matrix& a_worldMat, const Engine::Handle<Engine::Resource::Model>& a_modelHandle, const Handle<DynamicRaytracingData>& a_dynamicData, const RangeHandle<Resource::NodePoseMatrix>& a_nodeposeMatVec, const DXSM::Vector4& a_colorScale, const DXSM::Vector3& a_emissiveScale)
	{
		if (!m_upRayWorld)
		{
			m_upRayWorld = std::make_unique<RayWorld>();
		}

		// モデル登録
		m_upRayWorld->Register(a_world,a_worldMat,a_modelHandle,a_dynamicData,a_nodeposeMatVec,a_colorScale,a_emissiveScale);


		m_isCommit = false;
	}


	void Engine::Raytracing::RayEngine::CommitWorld(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList
	)
	{
		// レイワールドの作成
		if (!m_upRayWorld)
		{
			m_upRayWorld = std::make_unique<RayWorld>();
		}
		// ヒットグループ数がいるが仮置き
		m_upRayWorld->Init(a_pDevice,a_pCmdList,2);
	}

	void Engine::Raytracing::RayEngine::BegineFrame()
	{}

	void Engine::Raytracing::RayEngine::EndFrame()
	{
		m_upRayWorld->Clear();
		m_isCommit = false;
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