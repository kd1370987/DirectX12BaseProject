#pragma once

// 頂点のたどる順番を指定したバッファ

class IndexBuffer
{
public:

	IndexBuffer() = default;
	~IndexBuffer() = default;

	/// <summary>
	/// インデックスバッファの作成
	/// </summary>
	/// <param name="a_size">インデックス数</param>
	/// <param name="a_stride">インデックス一つ分のサイズ(2 or 4バイト)</param>
	/// <param name="a_pInitData">バッファのデータ・モデルなどのインデックス配列</param>
	/// <param name="a_format">インデックスのフォーマット(DXGI_FORMAT_R16_UINT or DXGI_FORMAT_R32_UINT)</param>
	/// <returns>作成に成功したらtrueを返す</returns>
	bool Create(
		size_t a_size, 
		size_t a_stride,
		const uint32_t* a_pInitData = nullptr,
		DXGI_FORMAT a_format = DXGI_FORMAT_R32_UINT
	);

	/// <summary>
	/// インデックスバッファビューを取得
	///	</summary>
	const D3D12_INDEX_BUFFER_VIEW& View() const;

	/// <summary>
	/// 登録されているインデックス数を取得
	/// </summary>
	UINT GetCount() const { return m_count; }

private:

	ComPtr<ID3D12Resource> m_pBuffer = nullptr;		// インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW m_view = {};			// インデックスバッファビュー

	DXGI_FORMAT m_format = DXGI_FORMAT_R32_UINT;	// インデックスフォーマット
	UINT m_count = 0;								// インデックス数

private:

	// コピー禁止
	IndexBuffer(const IndexBuffer&) = delete;
	void operator = (const IndexBuffer&) = delete;
};