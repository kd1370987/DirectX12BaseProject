#pragma once
#include "../MegaBuffer.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シーンに一つ持つような巨大なバッファ
	/// 静的なデータで毎フレーム変わるようなデータには向かない
	/// </summary>
	template<typename T>
	class MegaStructuredBuffer : public MegaBuffer
	{
	public:
		MegaStructuredBuffer() = default;
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
		RangeHandle<T> AllocateAndUpload(const T* a_pData, UINT a_count);

		/// <summary>
		/// 領域の解放
		/// </summary>
		/// <param name="a_handle"></param>
		void Free(const RangeHandle<T>& a_handle);

		/// <summary>
		/// バッファの更新
		/// </summary>
		/// <param name="a_currentFrameFence"></param>
		void Update(uint64_t a_currentFrameFence);

	private:
		bool m_isDrty = false;
		RangeAllocator<T> m_rangeAllocator;
	};
	template<typename T>
	inline bool MegaStructuredBuffer<T>::Create(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList, size_t a_elemetNum)
	{
		// アロケーターの作成
		m_rangeAllocator.Init(a_elemetNum);

		return MegaBuffer::Create(a_pDevice,a_pCmdList,a_elemetNum,sizeof(T));
	}
	template<typename T>
	inline RangeHandle<T> MegaStructuredBuffer<T>::AllocateAndUpload(const T* a_pData, UINT a_count)
	{
		// アロケーターから領域を確保
		auto _handle = m_rangeAllocator.AllocateRange(a_count);
		if (!_handle.IsValid()) return _handle;		// 容量不足

		// バイトオフセットとサイズを計算
		UINT _destOffsetBytes = _handle.startIndex * sizeof(T);
		UINT _sizeBytes = a_count * sizeof(T);

		// 基底クラスの隠蔽された関数を呼んで非同期アップロード
		UploadDataAsync(_destOffsetBytes, a_pData, _sizeBytes);

		return _handle;
	}

	template<typename T>
	inline void MegaStructuredBuffer<T>::Free(const RangeHandle<T>& a_handle)
	{
		if (!a_handle.IsValid()) return;

		// 基底クラスからフェンス値を取得し、遅延解放を予約
		uint64_t _currentFence = GetCurrentFenceValue();
		m_rangeAllocator.FreeRange(a_handle, _currentFence);
	}
	template<typename T>
	inline void MegaStructuredBuffer<T>::Update(uint64_t a_currentFrameFence)
	{
		m_rangeAllocator.UpdateFrees(a_currentFrameFence);
	}
}