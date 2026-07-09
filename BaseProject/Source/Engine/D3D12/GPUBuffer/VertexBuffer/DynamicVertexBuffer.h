#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	template<typename T>
	class DynamicVertexBuffer : public DynamicBuffer
	{
	public:

		DynamicVertexBuffer() = default;
		~DynamicVertexBuffer() override = default;
		NON_COPYABLE_MOVABLE(DynamicVertexBuffer);

		// 作成
		bool Create(D3D12::Device* a_pDevice,size_t a_elementNum);
		bool CreateAndUpload(D3D12::Device* a_pDevice,size_t a_elementNum, const void* a_pInitData);

		// アクセサ
		const D3D12_VERTEX_BUFFER_VIEW& GetView() const;

	private:

		D3D12_VERTEX_BUFFER_VIEW m_view = {};

	};
	template<typename T>
	inline bool DynamicVertexBuffer<T>::Create(D3D12::Device* a_pDevice, size_t a_elementNum)
	{
		// リソース作成
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;

		if (!DynamicBuffer::Create(a_pDevice,_desc))
		{
			assert(0 && "リソース作成失敗");
			return false;
		}

		// 頂点バッファビュー作成
		m_view = {};
		m_view.BufferLocation = GetGPUVirtualAddress();
		m_view.SizeInBytes = static_cast<UINT>(GetBufferSize());
		m_view.StrideInBytes = static_cast<UINT>(GetStrideSize());

		return true;
	}
	template<typename T>
	inline bool DynamicVertexBuffer<T>::CreateAndUpload(D3D12::Device* a_pDevice, size_t a_elementNum, const void* a_pInitData)
	{		
		// リソース作成
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;

		if (!DynamicBuffer::Create(a_pDevice, _desc))
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

		DynamicBuffer::UpdateData(a_pInitData,GetBufferSize());

		return true;
	}
	template<typename T>
	inline const D3D12_VERTEX_BUFFER_VIEW& DynamicVertexBuffer<T>::GetView() const
	{
		return m_view;
	}
}