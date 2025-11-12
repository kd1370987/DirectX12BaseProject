#include "IndexBuffer.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

IndexBuffer::IndexBuffer(size_t a_size, const uint32_t* a_pInitData)
{
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	D3D12_RESOURCE_DESC _desc = CD3DX12_RESOURCE_DESC::Buffer(a_size);	// リソースの設定

	// リソースを生成
	auto _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_prop,										// ヒートプロパティ
		D3D12_HEAP_FLAG_NONE,						// ヒープフラグ
		&_desc,										// リソース記述子（サイズ、フォーマット）
		D3D12_RESOURCE_STATE_GENERIC_READ,			// 初期のリソースステート
		nullptr,									// 最適化されたクリア値（テクスチャ用,Bufferならnullptr）
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())		// 出力のCOMポインタ受け取り
	);
	if (FAILED(_hr))
	{
		printf("[OnInit]　インデックスバッファのリソースの生成に失敗");
		return;
	}

	// インデックバッファのビュー設定
	m_view = {};
	m_view.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_view.Format = DXGI_FORMAT_R32_UINT;
	m_view.SizeInBytes = static_cast<UINT>(a_size);

	// マッピングする(GPUのVRAM上にあるリソースをCPUから直接触れるようにアドレスを紐づけること)
	if (a_pInitData != nullptr)
	{
		void* _ptr = nullptr;
		_hr = m_pBuffer->Map(0, nullptr, &_ptr);
		if (FAILED(_hr))
		{
			printf("[OnInit] インデックスバッファマッピングに失敗");
		}

		// インデックスデータをマッピング先に設定
		// CPUからGPUのバッファに直接アクセス
		memcpy(_ptr, a_pInitData, a_size);

		// マッピング解除(CPUとGPUの紐づけを解除)
		m_pBuffer->Unmap(0, nullptr);
	}
	m_isValid = true;
}

bool IndexBuffer::IsValid()
{
	return m_isValid;
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::View() const
{
	return m_view;
}
