#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/TextureManager/TextureManager.h"

Engine::Raytracing::RayWorld::RayWorld()
{

}

Engine::Raytracing::RayWorld::~RayWorld()
{
}

void Engine::Raytracing::RayWorld::Register(const DirectX::XMFLOAT4X4& a_worldMat, const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle)
{

	auto* _pDevice = D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	auto* _model = Engine::Resource::ModelManager::Instnace().GetModel(a_modelHandle);
	for (auto& _spMesh : _model->spMeshVec)
	{
		Engine::Raytracing::Instance _rayInst = {};
		_rayInst.worldMat = a_worldMat;
		_rayInst.pBLAS = _spMesh->GetBLAS();
		_rayInst.vertexHandle = _spMesh->GetSVertexBuff().GetHandle();
		_rayInst.indexHandle = _spMesh->GetSIndexBuff().GetHandle();
		//_rayInst.vertexHandle = _spMesh->GetVertexBuffer().GetHandle();
		//_rayInst.indexHandle = _spMesh->GetIndexBuffer().GetHandle();
		for (auto& _subset : _spMesh->GetSubsets())
		{
			_rayInst.pMaterial = &_model->materials[_subset.materialNumber];

			// 頂点用構造体バッファ作成
			auto& _vertexData = _spMesh->GetVertexData();
			std::vector<Vertex> _rtVertData = {};
			for (auto& _data : _vertexData)
			{
				Vertex _vert = {};
				_vert.uv = _data.uv;
				_rtVertData.push_back(_vert);
			}
			_rayInst.vertexBuffer.Create(_pDevice, _pCmdList,_rtVertData.size(), _rtVertData.data());
			SRVViewInit _viewInit = {};
			_viewInit.pResource = _rayInst.vertexBuffer.GetResource();
			_viewInit.pDesc = _rayInst.vertexBuffer.GetViewDesc();
			auto _handle = DescriptorHeapManager::Instance().AllocateSRVRange({ _viewInit })[0];
			_rayInst.vertexBuffer.SetHandle(_handle);

			// インデックス用構造体バッファ作成
			auto& _indexData = _spMesh->GetIndexData();
			_rayInst.indexBuffer.Create(_pDevice, _pCmdList, _indexData.size(),_indexData.data());
			_viewInit = {};
			_viewInit.pResource = _rayInst.indexBuffer.GetResource();
			_viewInit.pDesc = _rayInst.indexBuffer.GetViewDesc();
			_handle = DescriptorHeapManager::Instance().AllocateSRVRange({ _viewInit })[0];
			_rayInst.indexBuffer.SetHandle(_handle);

			//m_instanceVec.push_back(_rayInst);
			m_instanceVec.emplace_back(std::move(_rayInst));
		}
	}
}


void Engine::Raytracing::RayWorld::Commit()
{
	if(!m_upTLAS)
	{
		m_upTLAS = std::make_unique<TLAS>();
	}
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

	auto* _pDevice = D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// インスタンスデータ作成
	m_instanceDataBuffer.Create(_pDevice, _pCmdList, m_instanceVec.size(),_instanceDataVec.data());

	SRVViewInit _viewInit = {};
	_viewInit.pResource = m_instanceDataBuffer.GetResource();
	_viewInit.pDesc = m_instanceDataBuffer.GetViewDesc();
	auto _handle = DescriptorHeapManager::Instance().AllocateSRVRange({ _viewInit })[0];
	m_instanceDataBuffer.SetHandle(_handle);

	// マテリアルデータ作成
	std::vector<Material> _materialVec;
	for (auto& _instance : m_instanceVec)
	{
		Material _mate = {};
		_mate.baseColor = _instance.pMaterial->baseColor;
		auto _tex = Engine::Resource::TextureManager::Instance().GetTexture(_instance.pMaterial->baseColorTex);
		auto _srvH = _tex.GetSRV();
		_mate.baseTexSRV = static_cast<int>(_srvH.idx);
		_materialVec.push_back(_mate);
	}
	m_materialDataBuffer.Create(_pDevice, _pCmdList, m_instanceVec.size(),_materialVec.data());
	SRVViewInit _viewmateInit = {};
	_viewmateInit.pResource = m_materialDataBuffer.GetResource();
	_viewmateInit.pDesc = m_materialDataBuffer.GetViewDesc();
	auto _matehandle = DescriptorHeapManager::Instance().AllocateSRVRange({_viewmateInit})[0];
	m_materialDataBuffer.SetHandle(_matehandle);
}

void Engine::Raytracing::RayWorld::Update()
{
	auto* _pDevice = D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

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
	return DescriptorHeapManager::Instance().GetSRVGPUHandle(m_instanceDataBuffer.GetHandle());
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetMaterialSRV()
{
	return DescriptorHeapManager::Instance().GetSRVGPUHandle(m_materialDataBuffer.GetHandle());
}
