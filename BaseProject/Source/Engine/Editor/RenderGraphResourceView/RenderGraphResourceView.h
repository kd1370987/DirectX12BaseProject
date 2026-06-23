#pragma once

namespace Engine::Editor
{
	/// <summary>
	/// レンダーグラフで使われている一時リソースを描画するクラス
	/// </summary>
	class RenderGraphResourceView
	{
	public:

		RenderGraphResourceView() = default;
		~RenderGraphResourceView() = default;

		void Init();

		void Draw(UINT a_widht, UINT a_height);
	};
}