#pragma once

#include "../StaticBuffer/StaticBuffer.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シェーダーからバイトでアクセスされるシェーダー
	/// </summary>
	class StaticByteAddressBuffer : public StaticBuffer
	{
	public:

		~StaticByteAddressBuffer() override = default;
		NON_COPYABLE_MOVABLE(StaticByteAddressBuffer);

		/// <summary>
		/// 作成 : GPU処理が走る バリアは張っていないのでコピーコマンドリストでの実行が可能
		/// </summary>
		/// <param name="a_pDevice">デバイスポインタ</param>
		/// <param name="a_pCmdList">コマンドリストポインタ</param>
		/// <param name="a_elementNum">要素数</param>
		/// <param name="a_strideSize">1要素サイズ</param>
		/// <param name="a_pData">初期化データ</param>
		/// <returns>作成に成功すればtrue</returns>
		bool Create(
			D3D12::Device* a_pDevice, 
			D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_elementNum,
			size_t a_strideSize,
			const void* a_pData = nullptr
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
		
		/// <summary>
		/// 作成時に登録されたハンドルを返します
		/// </summary>
		const Handle<SRV>& GetSRVHandle() const;

	private:

		// シェーダーリソースビュー設定
		D3D12_SHADER_RESOURCE_VIEW_DESC m_view = {};
	};
}

