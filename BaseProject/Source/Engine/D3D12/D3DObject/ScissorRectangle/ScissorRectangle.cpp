#include "ScissorRectangle.h"

namespace Engine::D3D12
{

	void ScissorRectangle::Create(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
	{
		// シザー矩形
		// ビューポートに表示された画像のどこからどこまでを画面に映し出すのかの設定

		m_scissorRectangle.left = 0;
		m_scissorRectangle.right = a_frameBufferWidth;
		m_scissorRectangle.top = 0;
		m_scissorRectangle.bottom = a_frameBufferHeight;
	}

	const D3D12_RECT& ScissorRectangle::Get() const
	{
		return m_scissorRectangle;
	}
}
