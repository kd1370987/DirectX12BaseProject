#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	// クラス作成用データ
	struct StaticBufferDesc
	{
		size_t elementNum = 0;
		size_t strideSize = 0;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	};

	//==========================================================================================
	// 比較的更新頻度が少ないバッファ向け親クラス
	//
	// 中身はGPU専用(DEFAULT)のバッファに置き、CPUからはアップロードバッファを経由して送る。
	// 送り方は使い方で分ける。
	//
	//   UpdateData + Update … アップロードバッファは1本だけ。
	//                          作成時の初期化や、たまにしか書き換えないもの、
	//                          あるいは持ち主自体がフレームごとに別(RenderContext など)のもの向け
	//   UploadFrame        … 毎フレーム丸ごと書き換えるもの向け。
	//                          フレームの数だけ区画を持つアップロードバッファを経由する
	//
	// 1本のアップロードバッファを毎フレーム書き換えてはいけない。
	// CPUはGPUより最大 CPU_FRAME_COUNT-1 フレーム先を走っているので、
	// 前のフレームのコピーがまだ読んでいる中身を上書きしてしまう
	//==========================================================================================
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
			DescriptorHeapManager* a_pHeapManager,
			GraphicsCommandList* a_pCmdList,
			const StaticBufferDesc& a_desc,
			const void* a_pInitData
		);

		// 更新
		void Update(GraphicsCommandList* a_pCmdList);

		// データ更新
		void UpdateData(const void* a_data, size_t a_size) override;

		/// <summary>
		/// 毎フレーム中身を書き換える使い方の転送。
		/// 今のフレームの区画へ書き込んでから、その区画をGPUバッファへコピーするコマンドを積む。
		/// 区画用のアップロードバッファは初めて呼ばれたときに作る(使わないバッファは持たない)。
		/// UpdateData / Update とは別経路なので、同じバッファで混ぜないこと
		/// </summary>
		/// <param name="a_pCmdList">コピーを積むコマンドリスト</param>
		/// <param name="a_pData">書き込むデータ</param>
		/// <param name="a_sizeBytes">書き込むバイト数(バッファの大きさ以下)</param>
		/// <param name="a_frameIndex">今のCPUフレーム番号(0 ～ CPU_FRAME_COUNT-1)</param>
		void UploadFrame(
			GraphicsCommandList* a_pCmdList,
			const void* a_pData,
			size_t a_sizeBytes,
			UINT a_frameIndex
		);

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
		void CreateSRVInternal(D3D12::Device* a_pDevice, DescriptorHeapManager* a_pHeapManager);

		// GPUバッファへデータをコピー
		void CopyToGPU(GraphicsCommandList* a_pCmdList);

		// UploadFrame 用の区画つきアップロードバッファを作る
		bool CreateFrameUploadBuffer();

	protected:
		// 更新する用のバッファ
		GPUBuffer m_gpuBuffer;
		bool m_isDrty = false;

		// UploadFrame 用 : GetBufferSize() の区画を CPU_FRAME_COUNT 個並べたアップロードバッファ
		GPUBuffer m_frameUploadBuffer;
		std::byte* m_pFrameUploadMap = nullptr;		// 先頭(マップしたまま)
	};
}