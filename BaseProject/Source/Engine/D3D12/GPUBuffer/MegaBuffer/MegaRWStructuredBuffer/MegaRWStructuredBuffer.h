#pragma once
#include "../MegaBuffer.h"

namespace Engine::D3D12
{
	/// <summary>
	/// シーンに一つ持つような巨大なバッファ
	/// 静的なデータで毎フレーム変わるようなデータには向かない
	/// </summary>
	template<typename T>
	class MegaRWStructuredBuffer : public MegaBuffer
	{
	public:
		MegaRWStructuredBuffer() = default;
		~MegaRWStructuredBuffer() override = default;
		NON_COPYABLE_MOVABLE(MegaRWStructuredBuffer);

		/// <summary>
		/// バッファの作成
		/// </summary>
		/// <param name="a_pDevice">デバイス</param>
		/// <param name="a_elemetNum">要素数</param>
		/// <returns></returns>
		bool Create(
			D3D12::Device* a_pDevice,
			UINT a_elemetNum
		);

		/// <summary>
		/// // 中身のどこを使うかを割り当てて返す
		/// </summary>
		/// <param name="a_count">要素数</param>
		RangeHandle<T> Allocate(UINT a_count);

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
	inline bool MegaRWStructuredBuffer<T>::Create(D3D12::Device* a_pDevice, UINT a_elemetNum)
	{
		// アロケーターの作成
		m_rangeAllocator.Init(static_cast<uint32_t>(a_elemetNum));

		// バッファ作成
		GPUBufferDesc _desc = {};
		_desc.elementNum = static_cast<size_t>(a_elemetNum);
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		_desc.heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (!GPUBuffer::Create(a_pDevice, _desc))
		{
			assert(0 && "RWStructredBufferの作成に失敗");
			return false;
		}

		// 仕様書作成
		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_srvDesc.Buffer.FirstElement = 0;
		_srvDesc.Buffer.NumElements = a_elemetNum;
		_srvDesc.Buffer.StructureByteStride = sizeof(T);
		_srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// UAV
		D3D12_UNORDERED_ACCESS_VIEW_DESC _uavDesc = {};
		_uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		_uavDesc.Format = DXGI_FORMAT_UNKNOWN;					// ストラクチャードバッファは必ずUNKNOWN
		_uavDesc.Buffer.FirstElement = 0;
		_uavDesc.Buffer.NumElements = a_elemetNum;
		_uavDesc.Buffer.StructureByteStride = sizeof(T);
		_uavDesc.Buffer.CounterOffsetInBytes = 0;				// Append/Consumeは使わないので0
		_uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		// ハンドルをもらう
		m_srvHandle = AllocateSRV(a_pDevice, GetResource(), _srvDesc);
		m_uavHandle = AllocateUAV(a_pDevice, GetResource(), _uavDesc);

		return true;
	}

	template<typename T>
	inline RangeHandle<T> MegaRWStructuredBuffer<T>::Allocate(UINT a_count)
	{
		// アロケーターから領域を確保
		auto _handle = m_rangeAllocator.AllocateRange(a_count);
		if (!_handle.IsValid()) return _handle;		// 容量不足

		// バイトオフセットとサイズを計算
		UINT _destOffsetBytes = _handle.startIndex * sizeof(T);
		UINT _sizeBytes = a_count * sizeof(T);


		// 中身のどこを使うかを割り当てて返す
		return _handle;
	}

	template<typename T>
	inline void MegaRWStructuredBuffer<T>::Free(const RangeHandle<T>& a_handle)
	{
		if (!a_handle.IsValid()) return;

		// 今フレームがこの領域を参照している可能性があるため、
		// 今フレーム完了時のフェンス値でタグ付けして遅延解放を予約する
		m_rangeAllocator.FreeRange(a_handle, GetNextFenceValue());
	}

	template<typename T>
	inline void MegaRWStructuredBuffer<T>::Update(uint64_t a_currentFrameFence)
	{
		m_rangeAllocator.UpdateFrees(a_currentFrameFence);
	}

}