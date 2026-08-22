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

		// エディターを出したまま、シーンビューの中で遊ぶ(デバッグプレイ)。
		//
		// 見た目は Editor、操作は Game。
		//   ・エディターのパネルはそのまま描かれる(インスペクターで値を見ながら動かせる)
		//   ・フリーカメラの割り込みはしない(ゲームのカメラがそのまま映る)
		//   ・入力はゲームへ渡る(カーソル中央固定も working)
		// 抜けるのは Ctrl+P(Editor へ戻る)。
		//
		// ※ モードを見て分岐している箇所は「Game かどうか」で書かれていることが多い。
		//   足すときは Game / Editor のどちらの仲間なのかを必ず確かめること
		DebugPlay,
	};
}