#include "InputAxisForWindowsMouse.h"

#include "../../../Internal/InputContext.h"

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

	void InputAxisForWindowsMouse::Update(InputContext& a_inputContext)
	{
		bool _needCreateAxisState = true;
		bool _needUpdatePrevPos = true;

		// 軸固定モードで固定ボタンが押されているときは軸情報を作成し、軸の中心を更新しない
		if (m_spFixButton)
		{
			m_spFixButton->Update(a_inputContext);

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
			if(a_inputContext.isMouseLockToCenter)
			{
				// カーソルを中央へ戻しているため自前の座標差分は使えない
				// (戻した分まで拾ってしまう)ので、確定済みの移動量を使う。
				// スクリーン座標のYは下方向が正なので、上方向を正とする軸に合わせて反転する
				m_axis.x = static_cast<float>(a_inputContext.deltaX);
				m_axis.y = -static_cast<float>(a_inputContext.deltaY);
			}
			else
			{
				m_axis.x = float(_nowPos.x - m_prevMousePos.x);
				m_axis.y = float(m_prevMousePos.y - _nowPos.y);
			}
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