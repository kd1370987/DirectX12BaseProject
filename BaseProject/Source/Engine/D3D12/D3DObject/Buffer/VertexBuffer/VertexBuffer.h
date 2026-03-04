#pragma once

class VertexBuffer
{
public:

	VertexBuffer() = default;
	~VertexBuffer() = default;

	/// <summary>
	/// 頂点バッファ作成
	/// </summary>
	/// <param name="a_size">頂点数</param>
	/// <param name="a_stride">頂点一つ分のサイズ</param>
	/// <param name="a_pInitData">実際のデータ</param>
	/// <returns></returns>
	bool Create(
		size_t a_size, 
		size_t a_stride, 
		const void* a_pInitData
	);

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="a_count">頂点数</param>
	/// <param name="a_data">頂点配列データ</param>
	void Update(
		size_t a_count,
		const void* a_data
	);

	/// <summary>
	/// 頂点バッファビューを取得
	/// </summary>
	const D3D12_VERTEX_BUFFER_VIEW& View()const;				// 頂点バッファビューを取得

private:

	ComPtr<ID3D12Resource> m_pBuffer = nullptr;			// バッファ本体
	D3D12_VERTEX_BUFFER_VIEW m_view = {};				// 頂点バッファビュー

	// コピー禁止
	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};