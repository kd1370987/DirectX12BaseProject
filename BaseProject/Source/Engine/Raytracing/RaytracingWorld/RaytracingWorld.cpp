#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Raytracing
{
	Engine::Raytracing::RayWorld::RayWorld(){}
	Engine::Raytracing::RayWorld::~RayWorld(){}


	void Engine::Raytracing::RayWorld::Register(
		const DXSM::Matrix& a_worldMat,
		const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle
	)
	{
		m_isDrity = true;
		// レイワールドにインスタンスを登録する処理
		auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();

		// モデルのノードとメッシュを参照してインスタンスに変換
		auto* _model = Engine::Resource::ResourceManager::Instance().Get(a_modelHandle);
		if (!_model) return;

		auto& _nodes = _model->GetOriginalNodeVec();
		for (auto& _node : _nodes)		// ノードループ
		{
			for (auto& _meshIdx : _node.meshIndices)	// メッシュループ
			{
				auto _pMesh = _model->GetSPMeshVec()[_meshIdx].get();
				if (!_pMesh) continue;

				DXSM::Matrix _nodeMat = _node.worldTransform;


				// インスタンス作成
				Engine::Raytracing::Instance _rayInst = {};
				_rayInst.worldMat = _nodeMat * a_worldMat;
				if (!_pMesh->HasRtData()) continue;
				_rayInst.pBLAS = &_pMesh->GetRtData().blas;
				_rayInst.vertexHandle = _pMesh->GetRtData().structuredVertexBuffer.GetSRVHandle();
				_rayInst.indexHandle = _pMesh->GetRtData().structuredIndexBuffer.GetSRVHandle();
				for (auto& _subset : _pMesh->GetMetaData().subsets)
				{
					auto* _pMate = _model->GetMaterialVec()[_subset.materialNumber].get();
					if (!_pMate) continue;
	
					Material _mat = {};
					_mat.baseColor = _pMate->baseColor;
					_mat.metallic = _pMate->metallic;
					_mat.roughness = _pMate->roughness;
					_mat.emissive = _pMate->emissive;
					_mat.startIndexLocation = _subset.faceStart * 3;
					const auto* _Btex = Engine::Resource::ResourceManager::Instance().Get(_pMate->baseColorTex);
					_mat.baseIndex = _Btex->GetSRV().idx;// +100;
					const auto* _Mtex = Engine::Resource::ResourceManager::Instance().Get(_pMate->metaRoughTex);
					_mat.metaRoughnessIndex = _Mtex->GetSRV().idx;// +100;
					const auto* _Etex = Engine::Resource::ResourceManager::Instance().Get(_pMate->emissiveTex);
					_mat.emissiveIndex = _Etex->GetSRV().idx;// +100;
					const auto* _Ntex = Engine::Resource::ResourceManager::Instance().Get(_pMate->normalTex);
					_mat.normalIndex = _Ntex->GetSRV().idx;// +100;

					_rayInst.submeshMaterial.push_back(_mat);
				}
				_rayInst.pMesh = _pMesh;
				m_instanceVec.emplace_back(_rayInst);
			}
		}

		// コミットされていない状態に
		m_isCommit = false;
	}

	void Engine::Raytracing::RayWorld::Init(uint32_t a_hitGroupNum)
	{	
		// GPU実行のためキューリセット
		D3D12::D3D12Wrapper::Instance().CommandQueueReset();
		auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCmdList();
		
		// 仮置き
		UINT _maxInstanceNum = 1000;

		// レイワールド構築
		if (!m_upTLAS)
		{
			m_upTLAS = std::make_unique<TLAS>();
		}
		m_upTLAS->SetHitGroupNum(a_hitGroupNum);
		m_upTLAS->Create(_maxInstanceNum);

		// インスタンスデータ作成
		m_instanceDataVec.clear();
		m_instanceDataVec.resize(_maxInstanceNum);
		m_instanceDataBuffer.Create(_pDevice, *_pCmdList, _maxInstanceNum, m_instanceDataVec.data());

		// マテリアルデータ作成
		m_materialVec.clear();
		m_materialVec.resize(_maxInstanceNum);
		m_materialDataBuffer.Create(_pDevice, *_pCmdList, _maxInstanceNum, m_materialVec.data());

		// TLAS、構造体バッファはGPU操作が必要なため
		D3D12::D3D12Wrapper::Instance().CloseAndExecuteComdLists(_pCmdList);
	}


	void Engine::Raytracing::RayWorld::Commit()
	{
		auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCmdList();

		// TLAS更新
		m_upTLAS->Update(m_instanceVec);

		UINT _materialOffset = 0;
		// 構造体バッファ更新
		m_instanceDataVec = {};
		m_materialVec = {};
		for (auto& _instance : m_instanceVec)
		{
			InstanceData _data = {};
			_data.vertexSRVIndex = _instance.vertexHandle.idx;// +100;
			_data.indexSRVIndex = _instance.indexHandle.idx;// +100;
			_data.materialOffset = _materialOffset;
			m_instanceDataVec.push_back(_data);

			// オフセット更新
			for (auto& _mate : _instance.submeshMaterial)
			{
				m_materialVec.push_back(_mate);
				_materialOffset++;
			}
			
		}
		m_instanceDataBuffer.UpdateData((void*)m_instanceDataVec.data(), m_instanceDataVec.size() * sizeof(InstanceData));
		m_materialDataBuffer.UpdateData((void*)m_materialVec.data(), m_materialVec.size() * sizeof(Material));
		m_instanceDataBuffer.Update(*_pCmdList);
		m_materialDataBuffer.Update(*_pCmdList);
	}

	void Engine::Raytracing::RayWorld::Clear()
	{
		m_instanceVec.clear();
	}

	D3D12_GPU_VIRTUAL_ADDRESS Engine::Raytracing::RayWorld::GetTLAS()
	{
		return m_upTLAS->GetGPUAddress();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetSRVTLAS()
	{
		return m_upTLAS->GetGPUHandle();
	}

	Resource::Handle<D3D12::SRV> Engine::Raytracing::RayWorld::GetInstanceBufferSRV()
	{
		return m_instanceDataBuffer.GetSRVHandle();
	}

	Resource::Handle<D3D12::SRV> Engine::Raytracing::RayWorld::GetMaterialBufferSRV()
	{
		return m_materialDataBuffer.GetSRVHandle();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetInstanceDataSRV()
	{
		return D3D12::DescriptorHeapManager::Instance().GetGPU(m_instanceDataBuffer.GetSRVHandle());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetInstanceDataSRVCPU()
	{
		return D3D12::DescriptorHeapManager::Instance().GetCPU(m_instanceDataBuffer.GetSRVHandle());
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetMaterialSRV()
	{
		return D3D12::DescriptorHeapManager::Instance().GetGPU(m_materialDataBuffer.GetSRVHandle());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetMaterialSRVCPU()
	{
		return D3D12::DescriptorHeapManager::Instance().GetCPU(m_materialDataBuffer.GetSRVHandle());
	}
}