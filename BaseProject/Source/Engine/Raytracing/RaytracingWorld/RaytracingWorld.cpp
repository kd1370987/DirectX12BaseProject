#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/TextureManager/TextureManager.h"

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
	//auto* _model = Engine::Resource::ModelManager::Instnace().GetModel(a_modelHandle);
	auto* _model = Engine::Resource::ResourceManager::Instance().Get(a_modelHandle);
	if (!_model) return;

	auto& _nodes = _model->GetOriginalNodeVec();
	for (auto& _node : _nodes)		// ノードループ
	{
		for (auto& _meshIdx : _node.meshIndices)	// メッシュループ
		{
			//auto& _spMesh = _model->spMeshVec[_meshIdx];
			auto& _spMesh = _model->GetSPMeshVec()[_meshIdx];

			DXSM::Matrix _nodeMat = _node.worldTransform;

			// インスタンス作成
			Engine::Raytracing::Instance _rayInst = {};
			_rayInst.worldMat = _nodeMat * a_worldMat;
			_rayInst.pBLAS = _spMesh->GetBLAS();
			_rayInst.vertexHandle = _spMesh->GetSVertexBuff().GetHandle();
			_rayInst.indexHandle = _spMesh->GetSIndexBuff().GetHandle();
			for (auto& _subset : _spMesh->GetSubsets())
			{
				_rayInst.pMaterial = &_model->GetMaterialVec()[_subset.materialNumber];
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
	std::vector<InstanceData> _instanceDataVec = {};
	for (auto& _instance : m_instanceVec)
	{
		InstanceData _data = {};
		_data.vertexSRVIndex = _instance.vertexHandle.idx;
		_data.indexSRVIndex = _instance.indexHandle.idx;
		_instanceDataVec.push_back(_data);
	}

	auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();

	// インスタンスデータ作成
	m_instanceDataBuffer.Create(_pDevice, _pCmdList, m_instanceVec.size(), _instanceDataVec.data());

	auto _handle = D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(
		Engine::D3D12::D3D12Wrapper::Instance().GetDevice(),
		m_instanceDataBuffer.GetResource(),
		m_instanceDataBuffer.GetViewDesc()
	);
	m_instanceDataBuffer.SetHandle(_handle);

	// マテリアルデータ作成
	std::vector<Material> _materialVec;
	for (auto& _instance : m_instanceVec)
	{
		Material _mate = {};
		_mate.baseColor = _instance.pMaterial->baseColor;
		_mate.metallic = _instance.pMaterial->metallic;
		_mate.roughness = _instance.pMaterial->roughness;
		_mate.emissive = _instance.pMaterial->emissive;
		
		// マテリアルのインデックス取得
		auto& _tex = Engine::Resource::TextureManager::Instance().GetTexture(_instance.pMaterial->baseColorTex);
		_mate.baseIndex = _tex.GetSRV().idx;

		_materialVec.push_back(_mate);
	}
	m_materialDataBuffer.Create(_pDevice, _pCmdList, m_instanceVec.size(), _materialVec.data());

	_handle = D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(
		Engine::D3D12::D3D12Wrapper::Instance().GetDevice(),
		m_materialDataBuffer.GetResource(),
		m_materialDataBuffer.GetViewDesc()
	);
	m_materialDataBuffer.SetHandle(_handle);
}


void Engine::Raytracing::RayWorld::Commit()
{
	auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();

	// TLAS更新
	m_upTLAS->Update(m_instanceVec);
	
	// 構造体バッファ更新
	m_instanceDataBuffer.Update(_pDevice, _pCmdList);
	m_materialDataBuffer.Update(_pDevice, _pCmdList);
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
	return D3D12::DescriptorHeapManager::Instance().GetGPU(m_instanceDataBuffer.GetHandle());
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetMaterialSRV()
{
	return D3D12::DescriptorHeapManager::Instance().GetGPU(m_materialDataBuffer.GetHandle());
}
