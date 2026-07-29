#pragma once
namespace Engine
{
	// ウィンドウモード
	enum class EWindowMode : UINT
	{
		Windowed,		// ウィンドウモード
		FullScreen,		// フルスクリーンモード
		Borederless,	// ボーダレスモード
	};

	// フレームバッファ数
	enum : UINT
	{
		BACKBUFFER_COUNT = 3,	// バックバッファ数
		CPU_FRAME_COUNT = 3		// CPUカウント数
	};

	// ビルド構成
	enum class EBuildConfiguration : UINT
	{
		Debug,			// 最適化なし、デバッグ機能フル稼働
		Development,	// 最適化あり、エディター、プロファイラーなどの開発ツール有効
		Shipping		// リリース用、デバッグ機能、エディタ機能はすべて除外
	};

	// アプリケーションモード
	enum class EAppMode : UINT
	{
		Game,			// ゲームを遊ぶ時と同じ画面
		Editor,			// エディター画面
	};
}