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
			D3D12::GraphicsCommandList* a_pCmdList,
			size_t a_elemetNum,
			size_t a_strideSize
		);

	protected:
		// テンプレート派生クラスから呼ぶための生データ操作関数
		void UploadDataAsync(UINT a_destOffsetBytes, const void* a_pData, UINT a_sizeBytes);
		uint64_t GetCurrentFenceValue() const;
	protected:
		bool m_isDrty = false;
	};
}