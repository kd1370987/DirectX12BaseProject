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

		// フェンスイベント作成
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}
	void ResourceManager::Update()
	{
		WaitRender();

		// 転送中でなければ何もしない
		if (!m_isUploading) return;

		// GPUが指定したフェンス値まで処理を終えたかチェック
		if (m_upFence->GetCompletedValue() >= m_fenceValue)
		{
			for (auto* _pRes : m_uploadBufferVec)
			{
				if (_pRes) _pRes->Release();
			}
			m_uploadBufferVec.clear();
			// 完了
			m_isUploading = false;
		}
	}
	D3D12::CommandList* ResourceManager::GetCmdList()
	{
		return m_upCmdList.get();
	}
	void ResourceManager::CmdQueueReset()
	{
		m_upCmdAllocator->Reset();
		m_upCmdList->Reset(m_upCmdAllocator->Get());
	}
	void ResourceManager::SignalFence(ID3D12CommandQueue* a_pCmdQueue)
	{
		m_fenceValue++;

		a_pCmdQueue->Signal(
			m_upFence->GetFence(),
			m_fenceValue
		);
	}
	void ResourceManager::WaitRender()
	{
		if (m_upFence->GetCompletedValue() < m_fenceValue)
		{
			if (!m_upFence->SetEventOnCompletion(m_fenceValue, m_fenceEvent))
			{
				assert(0 && "フェンスイベントエラー");
				return;
			}
			// 待機処理
			if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE))
			{
				assert(0 && "待機処理エラー");
				return;
			}
		}
	}
	void ResourceManager::RegisterUploadBuffer(ID3D12Resource* a_pUploadBuffer)
	{
		m_uploadBufferVec.push_back(a_pUploadBuffer);
		m_isUploading = true;
	}
	ResourceManager::ResourceManager()
	{}
	ResourceManager::~ResourceManager()
	{}
}