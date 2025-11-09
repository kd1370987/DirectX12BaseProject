#pragma once

class VertexBuffer
{
public:

	// コンストラクタでバッファを生成
	VertexBuffer(size_t a_size, size_t a_stride, const void* a_pInitData);

	D3D12_VERTEX_BUFFER_VIEW View()const;				// 頂点バッファビューを取得
	bool IsValid();										// バッファの生成に成功したかを取得

private:

	bool m_isValid = false;								// バッファの生成に成功したかを取得
	ComPtr<ID3D12Resource> m_pBuffer = nullptr;			// バッファ本体
	D3D12_VERTEX_BUFFER_VIEW m_view = {};				// 頂点バッファビュー

	// コピー禁止
	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};