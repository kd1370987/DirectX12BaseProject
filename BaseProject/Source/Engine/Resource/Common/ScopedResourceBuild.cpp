#include "ScopedResourceBuild.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/MeshBufferAllocator/MeshBufferAllocator.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	ScopedResourceBuild::ScopedResourceBuild(bool a_useCopy, bool a_useCompute)
	{
		auto& _d3d = D3D12::D3D12Wrapper::Instance();

		// バッチを開く
		m_batch = _d3d.BeginAsyncBuildBatch(a_useCopy, a_useCompute);

		// コンテキストの組み立て
		m_context.pDevice = _d3d.GetDevice();
		m_context.pCopyCmdList = m_batch.pCopyCmdList;
		m_context.pComputeCmdList = m_batch.pComputeCmdList;
		m_context.pKeepAliveUploads = &m_batch.keepAliveResources;

		m_context.pResourceManager = &ResourceManager::Instance();
		m_context.pAssetDatabase = &AssetDatabase::Instance();

		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		m_context.pMeshBufferAllocator = _pGE ? _pGE->RefMeshBufferAllocator() : nullptr;
	}

	ScopedResourceBuild::~ScopedResourceBuild()
	{
		// スコープを抜けるところで実行
		D3D12::D3D12Wrapper::Instance().EndAsyncBuildBatch(m_batch);
	}
}
