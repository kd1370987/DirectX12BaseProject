#include "VertexBuffer.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

VertexBuffer::VertexBuffer(size_t a_size, size_t a_stride, const void* a_pInitData)
{
	// 頂点バッファの生成

	// 初期化情報
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(a_size);					// リソースの設定

	// リソースの生成
	auto _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		printf("頂点バッファリソースの生成に失敗");
		return;
	}

	// 頂点バッファビューの設定
	m_view.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_view.SizeInBytes = static_cast<UINT>(a_size);
	m_view.StrideInBytes = static_cast<UINT>(a_stride);

	// マッピングする
	if (a_pInitData != nullptr)
	{
		void* _ptr = nullptr;
		_hr = m_pBuffer->Map(0, nullptr, &_ptr);
		if (FAILED(_hr))
		{
			printf("頂点バッファマッピングに失敗");
			return;
		}

		// 頂点データをマッピング先に設定
		memcpy(_ptr, a_pInitData, a_size);

		// マッピング解除
		m_pBuffer->Unmap(0, nullptr);
	}

	// 作成成功
	m_isValid = true;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::View() const
{
	return m_view;
}

bool VertexBuffer::IsValid()
{
	return m_isValid;
}
