#pragma once
namespace Engine::Graphics::Pipeline
{
	class RenderGraph;

	/// <summary>
	/// レンダーグラフのコンパイルを管理する
	/// </summary>
	class RenderGraphCompiler
	{
	public:

		/// <summary>
		/// レンダーグラフをコンパイルして実行用データを作る
		/// </summary>
		/// <param name="a_pRenderGraph">レンダーグラフのポインタ</param>
		/// <returns>コンパイルに成功したら true</returns>
		bool Compile(RenderGraph* a_pRenderGraph);

	private:

		void ApplyLinks(RenderGraph* a_pRenderGraph);

	};
}