#pragma once
#include "../GPUBuffer.h"

#include "../../../Graphics/MeshBufferAllocator/IndexRangeAllocator/IndexRangeAllocator.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シーンに一つ持つような巨大なバッファ
	/// </summary>
	class MegaBuffer : GPUBuffer
	{
	public:

		~MegaBuffer() override = default;
		NON_COPYABLE_MOVABLE(MegaBuffer);

		/// <summary>
		/// 解放処理
		/// </summary>
		void Release() override;

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
			D3D12::GraphicsCommandList* a_pCmdList,
			size_t a_elemetNum,
			size_t a_strideSize
		);

		/// <summary>
		/// データの追加時
		/// </summary>
		/// <param name="a_pData">データ</param>
		/// <param name="a_count">確保数</param>
		Graphics::IndexRangeHandle AllocateAndUpdload(
			const void* a_pData,
			UINT a_count
		);

		/// <summary>
		/// 領域の解放
		/// </summary>
		/// <param name="a_handle">ハンドル</param>
		void Free(const Graphics::IndexRangeHandle& a_handle);

		/// <summary>
		/// 毎フレーム呼ばれる : 領域の結合
		/// </summary>
		void UpdateFrees();

		// ---- アクセサ ----
		const Handle<SRV>& GetSRVHandle() const { return m_srvHandle; }

	protected:

		// SRVの作成
		void CreateSRVInternal(D3D12::Device* a_pDevice);

	protected:
		bool m_isDrty = false;
		Handle<D3D12::SRV> m_srvHandle = {};

		Graphics::IndexRangeAllocator m_rangeAllocator = {};
	};
}