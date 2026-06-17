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
	enum
	{
		BACKBUFFER_COUNT = 3,	// バックバッファ数
		CPU_FRAME_COUNT = 3		// CPUカウント数
	};
}