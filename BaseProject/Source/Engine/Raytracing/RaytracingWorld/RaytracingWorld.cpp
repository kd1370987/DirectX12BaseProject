#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

Engine::Raytracing::RayWorld::RayWorld()
{

}

Engine::Raytracing::RayWorld::~RayWorld()
{
}

void Engine::Raytracing::RayWorld::Register(const DirectX::XMFLOAT4X4& a_worldMat, const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle)
{

	auto* _model = Engine::Resource::ModelManager::Instnace().GetModel(a_modelHandle);
	for (auto& _spMesh : _model->spMeshVec)
	{
		Engine::Raytracing::Instance _rayInst = {};
		_rayInst.worldMat = a_worldMat;
		_rayInst.pBLAS = _spMesh->GetBLAS();
		_rayInst.vertexHandle = _spMesh->GetVertexBuffer().GetHandle();
		_rayInst.indexHandle = _spMesh->GetIndexBuffer().GetHandle();
		m_instanceVec.push_back(_rayInst);
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
	m_instanceDataBuffer.Create(_pDevice,m_instanceVec.size(),_instanceDataVec.data());
	m_instanceDataBuffer.Update(_pDevice, _pCmdList);
}

D3D12_GPU_VIRTUAL_ADDRESS Engine::Raytracing::RayWorld::GetTLAS()
{
	return m_upTLAS->GetGPUAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetSRVTLAS()
{
	return m_upTLAS->GetGPUHandle();
}
