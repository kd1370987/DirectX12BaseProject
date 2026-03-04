#include "VertexBuffer.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool VertexBuffer::Create(
	size_t a_size,
	size_t a_stride, 
	const void* a_pInitData
)
{
	// 頂点バッファの生成
	size_t _bufferSize = a_size * a_stride;


	// 初期化情報
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_bufferSize);					// リソースの設定

	// リソースの生成
	auto _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "頂点バッファリソースの生成に失敗");
		return false;
	}

	// 頂点バッファビューの設定
	m_view.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_view.SizeInBytes = static_cast<UINT>(_bufferSize);
	m_view.StrideInBytes = static_cast<UINT>(a_stride);

	// マッピングする
	if (a_pInitData != nullptr)
	{
		void* _ptr = nullptr;
		_hr = m_pBuffer->Map(0, nullptr, &_ptr);
		if (FAILED(_hr))
		{
			assert(0 && "頂点バッファマッピングに失敗");
			return false;
		}

		// 頂点データをマッピング先に設定
		memcpy(_ptr, a_pInitData, _bufferSize);

		// マッピング解除
		m_pBuffer->Unmap(0, nullptr);
	}

	// 作成成功
	return true;
}

void VertexBuffer::Update(size_t a_count, const void* a_data)
{
	// バッファーサイズ
	size_t _bufferSize = a_count * m_view.StrideInBytes;

	// マップして更新
	void* _ptr = nullptr;
	m_pBuffer->Map(0,nullptr,&_ptr);

	std::memcpy(_ptr,a_data,_bufferSize);

	m_pBuffer->Unmap(0, nullptr);

	// ビュー情報修正
	m_view.SizeInBytes = static_cast<UINT>(_bufferSize);
}

const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::View() const
{
	return m_view;
}
