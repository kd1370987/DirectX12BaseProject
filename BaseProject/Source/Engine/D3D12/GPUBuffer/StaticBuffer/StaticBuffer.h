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
		StaticBuffer() = default;
		virtual ~StaticBuffer() override = default;
		NON_COPYABLE_MOVABLE(StaticBuffer);

		void Release() override;

		// 作成
		bool Create(
			D3D12::Device* a_pDevice, 
			GraphicsCommandList* a_pCmdList,
			const StaticBufferDesc& a_desc,
			const void* a_pInitData
		);

		// 更新
		void Update(GraphicsCommandList* a_pCmdList);

		// データ更新
		void UpdateData(const void* a_data, size_t a_size) override;

		/// <summary>
		/// バッファの指定した範囲だけを更新・GPUへ転送する（メガバッファ用）
		/// リソース遷移バリアがあるためメインのグラフィックスコマンドリストでの操作が必要
		/// </summary>
		/// <param name="a_pCmdList">GPU実行用のコマンドリスト</param>
		/// <param name="a_destOffsetBytes">書き込み先のバイトオフセット</param>
		/// <param name="a_pData">書き込むデータのポインタ</param>
		/// <param name="a_sizeBytes">書き込むデータのバイトサイズ</param>
		void UploadDataRange(
			D3D12::GraphicsCommandList* a_pCmdList,
			size_t a_destOffsetBytes,
			const void* a_pData,
			size_t a_sizeBytes
		);
		/// <summary>
		/// バッファの指定した範囲だけを更新・GPUへ転送する（メガバッファ用）
		/// リソース遷移バリアがあるためメインのグラフィックスコマンドリストでの操作が必要
		/// </summary>
		/// <param name="a_pCmdList">GPU実行用のコマンドリスト</param>
		/// <param name="a_startIndex">開始位置 : 内部でのサイズ計算はしてくれてるため純粋なインデックス</param>
		/// <param name="a_count">総数 : バイトサイズではなく、純粋な要素数</param>
		/// <param name="a_pData">データ</param>
		void UploadDataRange(
			D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_startIndex,
			UINT a_count,
			const void* a_pData
		);

		// 派生関数
		// ステート遷移
		void Barrier(D3D12::GraphicsCommandList* a_pCmdList, D3D12_RESOURCE_STATES a_nextState) override;

		// アクセサ
		ID3D12Resource* GetResource() const override;
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const override;

	protected:

		// SRVの作成
		void CreateSRVInternal(D3D12::Device* a_pDevice);

		// GPUバッファへデータをコピー
		void CopyToGPU(GraphicsCommandList* a_pCmdList);

	protected:
		// 更新する用のバッファ
		GPUBuffer m_gpuBuffer;
		bool m_isDrty = false;
	};
}