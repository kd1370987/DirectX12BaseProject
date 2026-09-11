#include "ScopedResourceBuild.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/MeshBufferAllocator/MeshBufferAllocator.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	ScopedResourceBuild::ScopedResourceBuild(bool a_useCopy, bool a_useCompute)
	{
		// バッチを開くのも転送を流すのもグラフィックスエンジン
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE)
		{
			ENGINE_ERRLOG(false, "リソースのビルドにグラフィックスエンジンがありません");
			return;
		}

		// バッチを開く
		m_batch = _pGE->BeginAsyncBuildBatch(a_useCopy, a_useCompute);

		// コンテキストの組み立て
		m_context.pDevice = _pGE->RefDevice();
		m_context.pCopyCmdList = m_batch.pCopyCmdList;
		m_context.pComputeCmdList = m_batch.pComputeCmdList;
		m_context.pKeepAliveUploads = &m_batch.keepAliveResources;

		m_context.pResourceManager = &ResourceManager::Instance();
		m_context.pAssetDatabase = &AssetDatabase::Instance();

		m_context.pGraphicsEngine = _pGE;
		m_context.pMeshBufferAllocator = _pGE->RefMeshBufferAllocator();
		m_context.pPassMetaRegistry = _pGE->RefPassMetaRegistry();

		// ビューの置き場。実体はグラフィックスエンジンの持ち物
		m_context.pHeapManager = _pGE->RefDescriptorHeapManager();
	}

	ScopedResourceBuild::~ScopedResourceBuild()
	{
		// スコープを抜けるところで実行
		if (!m_context.pGraphicsEngine) return;
		m_context.pGraphicsEngine->EndAsyncBuildBatch(m_batch);
	}
}
