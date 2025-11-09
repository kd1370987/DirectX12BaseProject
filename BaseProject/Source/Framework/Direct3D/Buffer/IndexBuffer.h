#pragma once

// 頂点のたどる順番を指定したバッファ

class IndexBuffer
{
public:

	IndexBuffer(size_t a_size,const uint32_t* a_pInitData = nullptr);
	bool IsValid();
	D3D12_INDEX_BUFFER_VIEW View() const;

private:
	bool m_isValid = false;
	ComPtr<ID3D12Resource> m_pBuffer;		// インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW m_view;			// インデックスバッファビュー

	// コピー禁止
	IndexBuffer(const IndexBuffer&) = delete;
	void operator = (const IndexBuffer&) = delete;

};