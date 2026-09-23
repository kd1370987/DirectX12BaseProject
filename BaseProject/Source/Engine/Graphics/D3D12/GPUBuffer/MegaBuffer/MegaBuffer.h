#pragma once
#include "../GPUBuffer.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シーンに一つ持つような巨大なバッファ
	/// </summary>
	class MegaBuffer : public GPUBuffer
	{
	public:
		MegaBuffer() = default;
		~MegaBuffer() override = default;
		NON_COPYABLE_MOVABLE(MegaBuffer);

		/// <summary>
		/// バッファの作成
		/// </summary>
		/// <param name="a_pDevice">デバイス</param>
		/// <param name="a_pCmdList">コマンドリスト</param>
		/// <param name="a_elemetNum">要素数</param>
		/// <param name="a_strideSize">要素サイズ</param>
		/// <returns></returns>
		bool Create(
			D3D12::Device* a_pDevice,
			DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList,
			size_t a_elemetNum,
			size_t a_strideSize
		);

	protected:
		// テンプレート派生クラスから呼ぶための生データ操作関数

		/// <summary>
		/// 渡されたコマンドリストへ転送コマンドを積むだけ
		/// 実行は呼び出し元(バッチを開いた側)の責任
		/// </summary>
		/// <param name="a_pCmdList">積み先のコピーコマンドリスト</param>
		/// <param name="a_keepAlive">転送完了まで生かしておく中間バッファの預け先</param>
		void RecordUploadData(
			GraphicsCommandList* a_pCmdList,
			std::vector<ComPtr<ID3D12Resource>>& a_keepAlive,
			UINT a_destOffsetBytes,
			const void* a_pData,
			UINT a_sizeBytes
		);

	private:
		// CPUデータを書き込んだUploadヒープを作る
		ComPtr<ID3D12Resource> CreateUploadBuffer(const void* a_pData, UINT a_sizeBytes);

	protected:
		bool m_isDrty = false;

		// 中間バッファを作るデバイス(借り物)。Create で受け取ったものを持ち続ける
		D3D12::Device* m_pDevice = nullptr;
	};
}