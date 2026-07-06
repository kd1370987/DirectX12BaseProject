#pragma once
#include "../MegaBuffer.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シーンに一つ持つような巨大なバッファ
	/// </summary>
	template<typename T>
	class MegaStructuredBuffer : MegaBuffer
	{
	public:

		~MegaStructuredBuffer() override = default;
		NON_COPYABLE_MOVABLE(MegaStructuredBuffer);

		/// <summary>
		/// バッファの作成
		/// </summary>
		/// <param name="a_pDevice">デバイス</param>
		/// <param name="a_pCmdList">コマンドリスト</param>
		/// <param name="a_elemetNum">要素数</param>
		/// <returns></returns>
		bool Create(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			size_t a_elemetNum
		);

		/// <summary>
		/// データのアップロード
		/// </summary>
		/// <param name="a_pData">型データ</param>
		/// <param name="a_count">要素数</param>
		Graphics::IndexRangeHandle AllocateAndUpload(const T* a_pData, UINT a_count);

	private:
		bool m_isDrty = false;
	};
	template<typename T>
	inline bool MegaStructuredBuffer<T>::Create(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList, size_t a_elemetNum)
	{
		return MegaBuffer::Create(a_pDevice,a_pCmdList,a_elemetNum,sizeof(T));
	}
	template<typename T>
	inline Graphics::IndexRangeHandle MegaStructuredBuffer<T>::AllocateAndUpload(const T* a_pData, UINT a_count)
	{
		return MegaBuffer::AllocateAndUpdload(a_pData, a_count);
	}
}