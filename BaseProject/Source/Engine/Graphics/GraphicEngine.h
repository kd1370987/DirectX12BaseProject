#pragma once

namespace Engine::Graphics
{
	// グラフィックスエンジンの初期化に必要な情報
	struct GraphicsEngineDesc
	{
		UINT width = 0;						// ウィンドウの横幅
		UINT height = 0;					// ウィンドウの縦幅
	};

	// グラフィックスエンジン
	class GraphicsEngine
	{
	public:

		// 初期化・解放
		void Init(const GraphicsEngineDesc& a_desc);
		void Release();

		// 更新
		void Update();

		// 描画
		void Draw();

	private:


	};
}