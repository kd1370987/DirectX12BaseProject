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

void Engine::Raytracing::RayWorld::Create()
{
	// 三角形構築
	m_testVert[0] = {-1,-1,-3};
	m_testVert[1] = {1,-1,-3};
	m_testVert[2] = {0,0,-3};
	m_testIndex[0] = 0;
	m_testIndex[1] = 1;
	m_testIndex[2] = 2;
	m_testVertBuff.Create(
		3,
		sizeof(Vertex),
		&m_testVert[0]
	);
	m_testVertBuff.CreateSRV();
	m_testIndexBuff.Create(
		3,
		sizeof(uint32_t),
		&m_testIndex[0]
	);
	m_testIndexBuff.CreateSRV();

	// BLAS構築
	m_upTestBLAS = std::make_unique<BLAS>();
	D3D12_RAYTRACING_GEOMETRY_DESC _geom = {};
	_geom.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	_geom.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	_geom.Triangles.VertexBuffer.StartAddress = m_testVertBuff.GetGPUVirtualAddress();
	_geom.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	_geom.Triangles.VertexCount = 3;
	_geom.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	_geom.Triangles.IndexBuffer = m_testIndexBuff.GetGPUVirtualAddress();
	_geom.Triangles.IndexCount = 3;
	_geom.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	_geom.Triangles.Transform3x4 = 0;
	
	m_upTestBLAS->Create(_geom);

	// TLAS構築
	m_upTestTLAS = std::make_unique<TLAS>();
	Instance _tlasInst = {};
	_tlasInst.pBLAS = m_upTestBLAS.get();
	_tlasInst.worldMat = DXSM::Matrix::Identity;
	_tlasInst.vertexHandle = m_testVertBuff.GetHandle();
	_tlasInst.indexHandle = m_testIndexBuff.GetHandle();
	m_upTestTLAS->Create(
		{_tlasInst}
	);
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
	return m_upTestTLAS->GetGPUAddress();
//	return m_upTLAS->GetGPUAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetSRVTLAS()
{
	return m_upTestTLAS->GetGPUHandle();
//	return m_upTLAS->GetGPUHandle();
}
