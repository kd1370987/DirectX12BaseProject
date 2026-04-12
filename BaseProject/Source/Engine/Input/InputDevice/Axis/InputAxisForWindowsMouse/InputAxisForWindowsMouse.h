#pragma once

#include "../InputAxisBase.h"

namespace Engine::Input
{
	class InputButtonBase;

	/// <summary>
	/// Windowsのマウス移動を利用した軸制御
	/// </summary>
	class InputAxisForWindowsMouse : public InputAxisBase
	{
	public:

		InputAxisForWindowsMouse() {};
		InputAxisForWindowsMouse(int a_fixCode);

		void PreUpdate() override;

		// マウスのマイフレームの移動量を使って、軸の入力状況を更新
		// 初めの１フレームのみ、０ベクトル
		void Update() override;

	private:
		POINT m_prevMousePos = {0,0};
		bool m_isBeginFrame = true;

		// マウスの一フレームの移動量ではなく固定された中心位置からの現在座標を軸情報として管理する。
		// マウスで議事ジョイスティックを操作したり、スマホの議事コントローラーのような
		// 動作をさせたいときに使用する。
		// 押している間、軸の中心位置が固定される
		std::shared_ptr<InputButtonBase> m_spFixButton = nullptr;
	};
}