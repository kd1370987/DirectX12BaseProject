#include "RaytracingEngine.h"

#include "Engine/Raytracing/RaytracingWorld/RaytracingWorld.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

void Engine::Raytracing::RayEngine::Create()
{
	m_outTex = Engine::Resource::TextureManager::Instance().CreateTexture(
		"RayOutTex",
		1280,
		720,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
	);
}

void Engine::Raytracing::RayEngine::Dispatch()
{
	
}


void Engine::Raytracing::RayEngine::RegistModel(const DirectX::XMFLOAT4X4& a_worldMat, const Engine::Resource::Handle<Resource::Model>& a_modelHandle)
{
	if(!m_upRayWorld)
	{
		m_upRayWorld = std::make_unique<RayWorld>();
	}

	// モデル登録
	m_upRayWorld->Register(a_worldMat,a_modelHandle);
}

void Engine::Raytracing::RayEngine::CommitWorld()
{
	m_upRayWorld->Commit();
}

Engine::Raytracing::RayEngine::RayEngine()
{}

Engine::Raytracing::RayEngine::~RayEngine()
{}
