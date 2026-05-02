#pragma once

namespace Engine::D3D12
{
	// シザー矩形
	class ScissorRectangle
	{
	public:

		// 作成
		void Create(
			UINT a_frameBufferWidth,
			UINT a_frameBufferHeight
		);

		// 取得
		const D3D12_RECT& Get() const;

	private:

		D3D12_RECT m_scissorRectangle = {};
	};

}