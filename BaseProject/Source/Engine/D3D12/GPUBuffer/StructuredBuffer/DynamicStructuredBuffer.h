#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	template<typename T>
	class DynamicStructuredBuffer : public DynamicBuffer
	{
	public:
		~DynamicStructuredBuffer() override = default;
		NON_COPYABLE_MOVABLE(DynamicStructuredBuffer);

		/// <summary>
		/// 作成
		/// </summary>
		/// <param name="a_pDevice">デバイス</param>
		/// <param name="a_maxElementCount">最大要素数</param>
		bool Create(D3D12::Device* a_pDevice, size_t a_maxElementCount);

		/// <summary>
		/// オフセットのリセット毎フレーム開始時に呼ぶ
		/// </summary>
		void ResetForNewFrame();

		/// <summary>
		/// データの書き込み : シェーダーに渡すためのオフセットを返す
		/// </summary>
		/// <param name="a_pData">書き込みデータの先頭ポインタ</param>
		/// <param name="a_count">要素数</param>
		/// <returns>メガバッファ内の開始インデックス</returns>
		uint32_t AllocateAndWrite(const T* a_pData, UINT a_count);

	private:
		uint32_t m_currentOffset = 0;
	};
	template<typename T>
	inline bool DynamicStructuredBuffer<T>::Create(D3D12::Device* a_pDevice, size_t a_maxElementCount)
	{
		// 親クラスの Create に渡す設定を構築
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_maxElementCount;
		_desc.strideSize = sizeof(T);
		// UPLOADヒープはUAVにはできないため、SRV専用としてフラグはNONE
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;

		// 仕様書作成
		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_srvDesc.Buffer.FirstElement = 0;
		_srvDesc.Buffer.NumElements = a_maxElementCount;
		_srvDesc.Buffer.StructureByteStride = sizeof(T);
		_srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// ハンドルをもらう
		m_srvHandle = AllocateSRV(a_pDevice, GetResource(), _srvDesc);

		// 親クラスの Create を呼ぶ（中でリソース確保、Map、SRV生成が行われる）
		return DynamicBuffer::Create(a_pDevice, _desc);
	}
	template<typename T>
	inline void DynamicStructuredBuffer<T>::ResetForNewFrame()
	{
		m_currentOffset = 0;
	}
	template<typename T>
	inline uint32_t DynamicStructuredBuffer<T>::AllocateAndWrite(const T* a_pData, UINT a_count)
	{
		if (m_currentOffset + a_count > m_elementNum)
		{
			ENGINE_ERRLOG(false, "動的メガバッファの容量オーバー");
			return 0;
		}

		// バイト単位で渡す
		size_t _offsetBytes = m_currentOffset * sizeof(T);
		size_t _sizeBytes = a_count * sizeof(T);
		UpdateDataOffset(a_pData, _sizeBytes, _offsetBytes);

		uint32_t _allocatedOffset = m_currentOffset;
		m_currentOffset += a_count;

		return _allocatedOffset;
	}
}