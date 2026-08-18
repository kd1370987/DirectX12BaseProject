#include "InputManager.h"

#include "../Internal/InputContext.h"

#include "../InputCollector/InputCollector.h"
#include "../InputDevice/Button/InputButtonBase.h"
#include "../InputDevice/Axis/InputAxisBase.h"

#include "../../MainEngine.h"
#include "../../Window/NativeWindow.h"
#include "../../Option/OptionManager.h"

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

		// ゲーム入力を受け付けてよい状態か。
		//
		// プレイモード以外(エディター操作中)は、キーもマウスもエディターのものなので
		// ゲーム側へは一切渡さない。ここで止めておけば、エディターで WASD を押しても
		// プレイヤーが動かないし、シーンビューでカーソルを振り回しても視点が回らない。
		//
		// 「入力が残らない」ためにも効く。取得側が常に無入力を返すので、
		// 入力フェーズのシステムが毎フレーム MoveIntent などへ 0 を書き込み、
		// 切り替え前の入力が押しっぱなしのまま固まらない。
		bool IsGameInputEnable()
		{
			return MainEngine::Instance().GetMode() == EAppMode::Game;
		}
	}

	void InputManager::Init()
	{
		// 保存されている設定を反映する
		// この時点ではまだウィンドウが生成されていないため、
		// 中心座標はここでは求めず、固定を行うフレームごとに実測する
		SetCursorCentered(Option::OptionManager::GetInstance().GetInputOption().isCursorLockedToCenter);
	}

	void InputManager::Update()
	{
		// マウスの固定化
		// エディタ操作中(Gameモード以外)は固定しない
		const auto _mode = MainEngine::Instance().GetMode();
		m_isCursorLockActive = (m_isCursorLockedToCenter && _mode == EAppMode::Game);

		if (m_isCursorLockActive)
		{
			SetCursorLock();
		}
		else
		{
			// 固定していない間の移動量は持ち越さない。
			// 次に固定を開始したフレームは基準を取り直す
			m_deltaX = 0;
			m_deltaY = 0;
			m_needCursorLockReset = true;
		}

		// 登録された入力でバスの更新を行う
		// (UIキャプチャ中でもデバイス自体は更新し、状態遷移の整合を保つ。
		//  実際に「入力なし」として扱うのは取得側で判定する)
		InputContext _context = {};
		_context.isMouseLockToCenter = m_isCursorLockActive;
		_context.deltaX = m_deltaX;
		_context.deltaY = m_deltaY;
		for (auto& _device : m_upInputDeviceMap)
		{
			_device.second->Update(_context);
		}
	}

	//======================================================================================
	// 溜まっている入力状態を捨てる
	//--------------------------------------------------------------------------------------
	// モードの切り替え時に呼ぶ。押しっぱなしのまま切り替えると、戻ってきたフレームが
	// 「押した瞬間(Press)」を挟まずに Hold から始まってしまう。
	// マウスは移動量と固定の基準も捨てる。切り替えの前後でカーソルが飛んでいるので、
	// そのまま差分を取ると1フレーム目に大きな視点移動が入ってしまう。
	//======================================================================================
	void InputManager::ResetInput()
	{
		m_deltaX = 0;
		m_deltaY = 0;
		m_needCursorLockReset = true;
		m_lockAnchorPos = {};

		for (auto& _device : m_upInputDeviceMap)
		{
			if (_device.second) _device.second->ResetInput();
		}
	}

	void InputManager::SetCursorCentered(bool a_enable)
	{
		m_isCursorLockedToCenter = a_enable;
		ENGINE_LOG("マウス座標固定化 : %s", BOOL_STR(a_enable));
	}

	// カーソルをクライアント領域の中心へ戻し、そのフレームの移動量を確定させる
	void InputManager::SetCursorLock()
	{
		POINT _center = {};
		if (!GetClientCenterPos(_center))
		{
			// ウィンドウが取れない・最小化中などは移動量なし
			m_deltaX = 0;
			m_deltaY = 0;
			m_needCursorLockReset = true;
			return;
		}

		POINT _nowPos = {};
		if (!GetCursorPos(&_nowPos))
		{
			m_deltaX = 0;
			m_deltaY = 0;
			m_needCursorLockReset = true;
			return;
		}

		if (m_needCursorLockReset)
		{
			// 固定を開始したフレームは基準が無いので移動量を出さない
			m_deltaX = 0;
			m_deltaY = 0;
			m_needCursorLockReset = false;
		}
		else
		{
			// 前フレームに「実際にカーソルを置いた位置」からの差分がこのフレームの移動量
			m_deltaX = _nowPos.x - m_lockAnchorPos.x;
			m_deltaY = _nowPos.y - m_lockAnchorPos.y;
		}

		// マウスを中央へ固定
		SetCursorPos(_center.x, _center.y);

		// 実際に移動できた座標を次フレームの基準にする。
		// 画面端でのクランプやDPIの丸めで要求どおりに移動できない場合があり、
		// 要求値を基準にすると毎フレーム同じ差分が残り続けて視点が震える
		if (!GetCursorPos(&m_lockAnchorPos))
		{
			m_lockAnchorPos = _center;
		}
	}

	// クライアント領域の中心をスクリーン座標で取得する
	bool InputManager::GetClientCenterPos(POINT& a_outPos) const
	{
		auto* _pWind = MainEngine::Instance().GetNativeWindow();
		if (!_pWind) return false;

		const HWND _hWnd = _pWind->GetWindowHandle();
		if (!_hWnd) return false;

		// ウィンドウの移動・リサイズ・フルスクリーン切替に追従するため毎回取り直す
		RECT _clientRect = {};
		if (!GetClientRect(_hWnd, &_clientRect)) return false;

		const LONG _width = _clientRect.right - _clientRect.left;
		const LONG _height = _clientRect.bottom - _clientRect.top;

		// 最小化中はクライアント領域が0になる
		if (_width <= 0 || _height <= 0) return false;

		// クライアント座標の中心を求めてからスクリーン座標へ変換する
		// (呼び出し側のメンバをそのまま渡すと、変換が累積して中心がずれていく)
		a_outPos.x = _clientRect.left + _width / 2;
		a_outPos.y = _clientRect.top + _height / 2;

		return (ClientToScreen(_hWnd, &a_outPos) != FALSE);
	}

	// 任意のアプリケーションボタンの入力状態を取得
	short InputManager::GetButtonState(std::string_view a_name) const
	{
		// プレイモード以外はゲーム入力を渡さない
		if (!IsGameInputEnable()) return InputButtonBase::EState::Free;

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
		// プレイモード以外はゲーム入力を渡さない
		if (!IsGameInputEnable()) return DXSM::Vector2(0.0f, 0.0f);

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