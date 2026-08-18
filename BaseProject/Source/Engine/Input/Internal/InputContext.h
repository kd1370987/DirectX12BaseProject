#pragma once

namespace Engine::Input
{
	// 入力デバイス間でやり取りされるデータ
	struct InputContext
	{
		// マウス制御
		bool isMouseLockToCenter = false;

		// このフレームのマウス移動量(感度を掛けた後)。
		// 元は WM_INPUT で届いたデバイスのカウントで、整数のまま扱うと
		// 感度を下げたときに端数が捨てられて動きが粗くなるため float で持つ。
		float deltaX = 0.0f;
		float deltaY = 0.0f;
	};
}