#pragma once

namespace Engine::D3D12
{
	class Viewport
	{
	public:

		// ビューポート作成
		// 画面サイズを入力
		void Create(
			float a_windowWidth,
			float a_windowHeight
		);

		// 取得
		const D3D12_VIEWPORT& Get() const;

	private:

		// ビューポート
		D3D12_VIEWPORT m_viewport = {};
	};
}