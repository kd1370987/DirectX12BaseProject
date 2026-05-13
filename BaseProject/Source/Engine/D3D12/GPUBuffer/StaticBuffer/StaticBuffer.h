#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	// 前方宣言
	class CommandList;

	// クラス作成用データ
	struct StaticBufferDesc
	{
		size_t elementNum = 0;
		size_t strideSize = 0;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	};

	// 比較的更新頻度が少ないバッファ向け親クラス
	// 更新が少しでもある可能性がある使い方の時はマイフレームアップデートを呼ぶ
	class StaticBuffer : public DynamicBuffer
	{
	public:

		virtual ~StaticBuffer() override = default;

		// 作成
		bool Create(
			ID3D12Device* a_pDevice, 
			CommandList& a_cmdList,
			const StaticBufferDesc& a_desc,
			const void* a_pInitData
		);

		// 更新
		void Update(CommandList& a_cmdList);

		// データ更新
		void UpdateData(const void* a_data, size_t a_size) override;

		// 派生関数
		// ステート遷移
		void Barrier(CommandList& a_cmdList, D3D12_RESOURCE_STATES a_nextState) override;

		// アクセサ
		ID3D12Resource* GetResource() const override;
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const override;

	protected:

		// SRVの作成
		void CreateSRVInternal(ID3D12Device* a_pDevice);

		// GPUバッファへデータをコピー
		void CopyToGPU(CommandList& a_cmdList);

	protected:
		// 更新する用のバッファ
		GPUBuffer m_gpuBuffer;
		bool m_isDrty = false;

		Resource::Handle<D3D12::SRV> m_srvHandle = {};
	};
}