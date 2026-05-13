#pragma once

#include "../GPUBuffer.h"

namespace Engine::D3D12
{
	template<typename T>
	class VertexBuffer : public GPUBuffer
	{
	public:

		VertexBuffer() = default;
		~VertexBuffer() override = default;

		// 作成
		bool Create(ID3D12Device* a_pDevice,size_t a_elementNum, const void* a_pInitData = nullptr);

		// SRV作成
		void CreateSRV(ID3D12Device* a_pDevice);

		// アクセサ
		const D3D12_VERTEX_BUFFER_VIEW& GetView() const;

	private:

		D3D12_VERTEX_BUFFER_VIEW m_view = {};

	};
	template<typename T>
	inline bool VertexBuffer<T>::Create(ID3D12Device* a_pDevice, size_t a_elementNum, const void* a_pInitData)
	{
		// リソース作成
		GPUBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;
		_desc.heapType = D3D12_HEAP_TYPE_UPLOAD;

		if (!GPUBuffer::Create(a_pDevice,_desc))
		{
			assert(0 && "リソース作成失敗");
			return false;
		}

		// 頂点バッファビュー作成
		m_view = {};
		m_view.BufferLocation = GetGPUVirtualAddress();
		m_view.SizeInBytes = static_cast<UINT>(GetBufferSize());
		m_view.StrideInBytes = static_cast<UINT>(GetStrideSize());

		// 初期化データがあればマップ
		if (!a_pInitData) return true;
		
		Write(&a_pInitData);

		return true;
	}
	template<typename T>
	inline void VertexBuffer<T>::CreateSRV(ID3D12Device* a_pDevice)
	{
		GPUBuffer::CreateSRVInternal(a_pDevice);
	}
	template<typename T>
	inline const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer<T>::GetView() const
	{
		return m_view;
	}
}