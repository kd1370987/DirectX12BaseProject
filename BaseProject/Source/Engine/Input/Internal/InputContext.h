#pragma once

namespace Engine::Input
{
	// 入力デバイス間でやり取りされるデータ
	struct InputContext
	{
		// マウス制御
		bool isMouseLockToCenter = false;
		int deltaX = 0;
		int deltaY = 0;
	};
}