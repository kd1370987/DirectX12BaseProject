#include "InputManager.h"

#include "../InputCollector/InputCollector.h"
#include "../InputDevice/Button/InputButtonBase.h"
#include "../InputDevice/Axis/InputAxisBase.h"
namespace Engine::Input
{
	namespace
	{
		// エディタのテキスト入力欄にフォーカスがあるか(=文字入力中か)。
		// この間はゲーム側の入力を無効化し、プレイヤー操作やシーン遷移が
		// 誤って走らないようにする。
		//
		// 注意: WantCaptureKeyboard は NavEnableKeyboard 有効時、ImGuiウィンドウに
		//       フォーカスがあるだけで常時 true になり得る(ドッキング型エディタでは
		//       ほぼ常時ブロックされてしまう)。そのため、テキスト入力中だけ true になる
		//       WantTextInput を使う。
		bool IsUICapturingInput()
		{
			// ImGuiコンテキストが無い(エディタ無効時など)なら何もブロックしない
			if (ImGui::GetCurrentContext() == nullptr) return false;
			return ImGui::GetIO().WantTextInput;
		}
	}

	void InputManager::Update()
	{
		// 登録された入力でバスの更新を行う
		// (UIキャプチャ中でもデバイス自体は更新し、状態遷移の整合を保つ。
		//  実際に「入力なし」として扱うのは取得側で判定する)
		for (auto& _device : m_upInputDeviceMap)
		{
			_device.second->Update();
		}
	}

	// 任意のアプリケーションボタンの入力状態を取得
	short InputManager::GetButtonState(std::string_view a_name) const
	{
		// エディタ操作中(テキスト入力など)はゲーム入力を無効化
		if (IsUICapturingInput()) return InputButtonBase::EState::Free;

		short _buttonState = InputButtonBase::EState::Free;
		for (auto& _device : m_upInputDeviceMap)
		{
			// 有効な時のみ入力に影響を与える
			if (_device.second->GetActiveState() == InputCollector::EActiveState::Enable)
			{
				_buttonState |= _device.second->GetButtonState(a_name);
			}
		}
		return _buttonState;
	}

	// 任意のアプリケーションボタンが押されていない状態か判定
	bool InputManager::IsFree(std::string_view a_name) const
	{
		return (GetButtonState(a_name) == InputButtonBase::EState::Free);
	}
	bool InputManager::IsPress(std::string_view a_name) const
	{
		return (GetButtonState(a_name) & InputButtonBase::EState::Press);
	}
	bool InputManager::IsHold(std::string_view a_name) const
	{
		return (GetButtonState(a_name) & InputButtonBase::EState::Hold);
	}
	bool InputManager::IsRelease(std::string_view a_name) const
	{
		return (GetButtonState(a_name) & InputButtonBase::EState::Release);
	}

	// 任意の軸の入力状態を取得
	// 指定した入力デバイスの任意の軸の入力状d態を2次元ベクトルで取得する
	DXSM::Vector2 InputManager::GetAxisState(std::string_view a_name) const
	{
		// エディタ操作中(テキスト入力など)はゲーム入力を無効化
		if (IsUICapturingInput()) return DXSM::Vector2(0.0f, 0.0f);

		float _leftValue = 0.0f;
		float _rightValue = 0.0f;
		float _topValue = 0.0f;
		float _bottomValue = 0.0f;

		for (auto& _collector : m_upInputDeviceMap)
		{
			// 有効な時のみ入力に影響を与える
			if (_collector.second->GetActiveState() == InputCollector::EActiveState::Enable)
			{
				DXSM::Vector2 _nowDeviceAxis = {};
				_nowDeviceAxis = _collector.second->GetAxisState(a_name);

				// 入力がなければスキップ
				if (_nowDeviceAxis.LengthSquared() == 0.0f) continue;

				// 左右の入力をX軸数値で判定
				if (_nowDeviceAxis.x < 0.0f)
				{
					// 左なら最小値を保持
					_leftValue = std::min(_nowDeviceAxis.x, _leftValue);
				}
				else
				{
					// 右なら最大値を保持
					_rightValue = std::max(_nowDeviceAxis.x,_rightValue);
				}

				// 上下の入力をY軸数値で判定
				if (_nowDeviceAxis.y < 0.0f)
				{
					// 下なら最小値を保持
					_bottomValue = std::min(_nowDeviceAxis.y ,_bottomValue);
				}
				else
				{
					// 上なら最大値を保持
					_topValue = std::max(_nowDeviceAxis.y,_topValue);
				}
			}
		}

		// 最終的に左右と上下の入力値をそれぞれ合成したものを出力
		return DXSM::Vector2(_leftValue + _rightValue,_topValue + _bottomValue);
	}

	// 入力コレクター(入力デバイス)の追加
	void InputManager::AddDevice(std::string_view a_name, InputCollector* a_pInputDevice)
	{
		std::unique_ptr<InputCollector> _upNewDevice(a_pInputDevice);
		AddDevice(a_name,std::move(_upNewDevice));
	}
	void InputManager::AddDevice(std::string_view a_name, std::unique_ptr<InputCollector> a_upInputDevice)
	{
		m_upInputDeviceMap[a_name.data()] = std::move(a_upInputDevice);
		return;
	}

	const std::unique_ptr<InputCollector>& InputManager::GetDevice(std::string_view a_name) const
	{
		auto _device = m_upInputDeviceMap.find(a_name.data());

		if (_device == m_upInputDeviceMap.end())
		{
			assert(0 && "未登録のデバイスです");
		}
		return _device->second;
	}
	std::unique_ptr<InputCollector>& InputManager::RefDevice(std::string_view a_name)
	{
		auto _device = m_upInputDeviceMap.find(a_name.data());

		if (_device == m_upInputDeviceMap.end())
		{
			assert(0 && "未登録のデバイスです");
		}
		return _device->second;
	}

	void InputManager::Release()
	{
		m_upInputDeviceMap.clear();
	}
	InputManager::InputManager()
	{}
	InputManager::~InputManager()
	{
		Release();
	}
}