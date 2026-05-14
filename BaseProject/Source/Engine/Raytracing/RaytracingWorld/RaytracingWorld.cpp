#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

Engine::Raytracing::RayWorld::RayWorld()
{

}

Engine::Raytracing::RayWorld::~RayWorld()
{
}

void Engine::Raytracing::RayWorld::Register(
	const DXSM::Matrix& a_worldMat,
	const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle
)
{
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
			_rayInst.pBLAS = _pMesh->GetBLAS();
			_rayInst.vertexHandle = _pMesh->GetSVertexBuff().GetSRVHandle();
			_rayInst.indexHandle = _pMesh->GetSIndexBuff().GetSRVHandle();
			for (auto& _subset : _pMesh->GetSubsets())
			{
				auto* _pMate = _model->GetMaterialVec()[_subset.materialNumber].get();
				if (!_pMate) continue;
				_rayInst.pMaterial = _pMate;
				m_instanceVec.emplace_back(_rayInst);
			}
		}
	}

	// コミットされていない状態に
	m_isCommit = false;
}

void Engine::Raytracing::RayWorld::Init(uint32_t a_hitGroupNum)
{
	if (!m_upTLAS)
	{
		m_upTLAS = std::make_unique<TLAS>();
	}
	m_upTLAS->SetHitGroupNum(a_hitGroupNum);
	m_upTLAS->Create(m_instanceVec);

	// 構造体バッファ作成
	m_instanceDataVec = {};
	for (auto& _instance : m_instanceVec)
	{
		InstanceData _data = {};
		_data.vertexSRVIndex = _instance.vertexHandle.idx;
		_data.indexSRVIndex = _instance.indexHandle.idx;
		m_instanceDataVec.push_back(_data);
	}

	auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCmdList();

	// インスタンスデータ作成
	m_instanceDataBuffer.Create(_pDevice, *_pCmdList, 1000, m_instanceDataVec.data());

	// マテリアルデータ作成
	m_materialVec = {};
	for (auto& _instance : m_instanceVec)
	{
		Material _mate = {};
		_mate.baseColor = _instance.pMaterial->baseColor;
		_mate.metallic = _instance.pMaterial->metallic;
		_mate.roughness = _instance.pMaterial->roughness;
		_mate.emissive = _instance.pMaterial->emissive;
		
		// マテリアルのインデックス取得
		const auto* _tex = Engine::Resource::ResourceManager::Instance().Get(_instance.pMaterial->baseColorTex);
		_mate.baseIndex = _tex->GetSRV().idx;

		m_materialVec.push_back(_mate);
	}
	m_materialDataBuffer.Create(_pDevice, *_pCmdList, 1000, m_materialVec.data());
}


void Engine::Raytracing::RayWorld::Commit()
{
	auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCmdList();

	// TLAS更新
	m_upTLAS->Update(m_instanceVec);
	
	// 構造体バッファ更新
	m_instanceDataVec = {};
	for (auto& _instance : m_instanceVec)
	{
		InstanceData _data = {};
		_data.vertexSRVIndex = _instance.vertexHandle.idx;
		_data.indexSRVIndex = _instance.indexHandle.idx;
		m_instanceDataVec.push_back(_data);
	}
	m_instanceDataBuffer.UpdateData((void*)m_instanceDataVec.data(),m_instanceDataBuffer.GetBufferSize());
	m_materialVec = {};
	for (auto& _instance : m_instanceVec)
	{
		Material _mate = {};
		_mate.baseColor = _instance.pMaterial->baseColor;
		_mate.metallic = _instance.pMaterial->metallic;
		_mate.roughness = _instance.pMaterial->roughness;
		_mate.emissive = _instance.pMaterial->emissive;

		// マテリアルのインデックス取得
		const auto* _tex = Engine::Resource::ResourceManager::Instance().Get(_instance.pMaterial->baseColorTex);
		_mate.baseIndex = _tex->GetSRV().idx;

		m_materialVec.push_back(_mate);
	}
	m_materialDataBuffer.UpdateData((void*)m_materialVec.data(),m_materialDataBuffer.GetBufferSize());
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
