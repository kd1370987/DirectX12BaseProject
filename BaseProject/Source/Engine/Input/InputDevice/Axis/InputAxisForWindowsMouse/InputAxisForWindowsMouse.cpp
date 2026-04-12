#include "InputAxisForWindowsMouse.h"

#include "../../Button/InputButtonForWindows/InputButtonForWindows.h"

namespace Engine::Input
{
	InputAxisForWindowsMouse::InputAxisForWindowsMouse(int a_fixCode)
	{
		m_spFixButton = std::make_shared<InputButtonForWindows>(a_fixCode);
	}

	void InputAxisForWindowsMouse::PreUpdate()
	{
		if (!m_spFixButton) return;

		// 軸キーがあれば
		m_spFixButton->PreUpdate();
	}

	void InputAxisForWindowsMouse::Update()
	{
		bool _needCreateAxisState = true;
		bool _needUpdatePrevPos = true;

		// 軸固定モードで固定ボタンが押されているときは軸情報を作成し、軸の中心を更新しない
		if (m_spFixButton)
		{
			m_spFixButton->Update();

			if (m_spFixButton->GetState())
			{
				_needUpdatePrevPos = false;
			}
			else
			{
				_needCreateAxisState = false;
			}
		}

		// 現在のマウス座標取得
		POINT _nowPos = {};
		GetCursorPos(&_nowPos);

		// 開始フレームでない & 軸情報の生成を必要とするとき
		if (!m_isBeginFrame && _needCreateAxisState)
		{
			m_axis.x = float(_nowPos.x - m_prevMousePos.x);
			m_axis.y = float(m_prevMousePos.y - _nowPos.y);
		}
		else
		{
			m_axis = DXSM::Vector2::Zero;
		}

		if (_needUpdatePrevPos)
		{
			// 座標を保持、次回以降はこの座標との差で移動量を求める
			m_prevMousePos = _nowPos;
		}

		m_isBeginFrame = false;
	}
}