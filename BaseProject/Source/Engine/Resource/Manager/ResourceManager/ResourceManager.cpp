#include "ResourceManager.h"

// D3D12オブジェクト
#include "../../../D3D12/D3DObject/CommandAllocator/CommandAllocator.h"
#include "../../../D3D12/D3DObject/CommandList/CommandList.h"
#include "../../../D3D12/D3DObject/Fence/Fence.h"

namespace Engine::Resource
{
	void ResourceManager::Init(ID3D12Device* a_pDevice, ID3D12CommandQueue* a_copyQueue)
	{
		m_pCopyCmdQueue = a_copyQueue;

		// D3Dオブジェクトの作成
		// コマンドアロケーターの作成
		m_upCmdAllocator = std::make_unique<D3D12::CommandAllocator>();
		m_upCmdAllocator->Create(a_pDevice,D3D12_COMMAND_LIST_TYPE_DIRECT);

		// コマンドリスト作成
		m_upCmdList = std::make_unique<D3D12::CommandList>();
		m_upCmdList->Create(a_pDevice,m_upCmdAllocator->Get());

		// フェンス作成
		m_upFence = std::make_unique<D3D12::Fence>();
		m_upFence->Create(a_pDevice);

		// フェンス目標値設定
		m_fenceValue = 0;
	}
	void ResourceManager::Update()
	{
		// 転送中でなければ何もしない
		if (!m_isUploading) return;

		// GPUが指定したフェンス値まで処理を終えたかチェック
		if (m_upFence->GetCompletedValue() >= m_fenceValue)
		{
			// 完了
			m_isUploading = false;
		}
	}
}