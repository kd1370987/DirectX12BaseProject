#pragma once

class Viewport
{
public:

	/// <summary>
	/// ビューポート作成
	/// </summary>
	/// <param name="a_frameBufferWidth">幅</param>
	/// <param name="a_frameBufferHeight">高さ</param>
	void Create(
		UINT a_frameBufferWidth,
		UINT a_frameBufferHeight
	);

	/// <summary>
	/// ビューポート取得
	/// </summary>
	/// <returns>ビューポート</returns>
	const D3D12_VIEWPORT& Get() const 
	{ 
		return m_viewport; 
	}

private:

	D3D12_VIEWPORT m_viewport = {};
};