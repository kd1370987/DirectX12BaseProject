#pragma once

class ScissorRectangle
{
public:

	/// <summary>
	/// シザー矩形作成
	/// </summary>
	/// <param name="a_frameBufferWidth">横幅</param>
	/// <param name="a_frameBufferHegiht">縦</param>
	void Create(
		UINT a_frameBufferWidth, 
		UINT a_frameBufferHeight
	);

	/// <summary>
	/// シザー矩形参照
	/// </summary>
	/// <returns>シザー矩形アドレス</returns>
	const D3D12_RECT& Get() const 
	{
		return m_scissorRectangle; 
	}

private:

	D3D12_RECT m_scissorRectangle;
};