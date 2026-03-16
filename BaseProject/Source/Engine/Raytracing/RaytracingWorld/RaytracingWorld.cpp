#include "RaytracingWorld.h"

#include "../TLAS/TLAS.h"

#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

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
}

D3D12_GPU_VIRTUAL_ADDRESS Engine::Raytracing::RayWorld::GetTLAS()
{
	return m_upTLAS->GetGPUAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::RayWorld::GetSRVTLAS()
{
	return m_upTLAS->GetGPUHandle();
}
