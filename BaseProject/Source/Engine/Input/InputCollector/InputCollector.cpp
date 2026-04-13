#include "InputCollector.h"
#include "../InputDevice/Button/InputButtonBase.h"
#include "../InputDevice/Axis/InputAxisBase.h"

namespace Engine::Input
{
	InputCollector::InputCollector()
	{}
	InputCollector::~InputCollector()
	{}
	void InputCollector::Update()
	{
		// 更新前の準備
		{
			for (auto& _button : m_spButtonMap)
			{
				_button.second->PreUpdate();
			}
			for (auto& _axis : m_spAxisMap)
			{
				_axis.second->PreUpdate();
			}
		}

		// 有効 or 監視 : ボタンの状態を更新
		if (GetActiveState() != EActiveState::Disable)
		{
			for (auto& _button : m_spButtonMap)
			{
				_button.second->Update();
			}
			for(auto& _axis : m_spAxisMap)
			{
				_axis.second->Update();
			}
		}
		// 無効 : すべて入力されていない状態に更新
		else
		{
			for (auto& _button : m_spButtonMap)
			{
				_button.second->NoInput();
			}
			for (auto& _axis : m_spAxisMap)
			{
				_axis.second->NoInput();
			}
		}
	}

	// 何かしらの入力検知したときにtureを返す
	bool InputCollector::IsSomethigInput()
	{
		for (auto& _button : m_spButtonMap)
		{
			if (!_button.second) continue;

			// 入力を受けていたらTrue
			if (_button.second->GetState() != InputButtonBase::EState::Free) return true;
		}

		for (auto& _axis : m_spAxisMap)
		{
			if (!_axis.second) continue;

			// 入力を受けていたらTrue
			if (_axis.second->GetState().LengthSquared() != 0.0f) return true;
		}

		// なんの入力も検知で着なかった
		return false;
	}

	// 任意のアプリケーションボタンの入力情報取得
	short InputCollector::GetButtonState(std::string_view a_name) const
	{
		const std::shared_ptr<InputButtonBase>& _spButton = GetButton(a_name);

		if (!_spButton)
		{
			return InputButtonBase::EState::Free;
		}

		return _spButton->GetState();
	}

	// 任意の軸の入力情報取得
	DXSM::Vector2 InputCollector::GetAxisState(std::string_view a_name) const
	{
		const std::shared_ptr<InputAxisBase>& _spAxis = GetAxis(a_name);

		if (!_spAxis)
		{
			return DXSM::Vector2::Zero;
		}

		return _spAxis->GetState();
	}

	// アプリケーションボタンの追加
	// 生ポインタの追加関数は必ず、newした生ポインタを引数として渡すこと
	void InputCollector::AddButton(std::string_view a_name, InputButtonBase* a_pButton)
	{
		AddButton(a_name.data(), std::shared_ptr<InputButtonBase>(a_pButton));
	}
	void InputCollector::AddButton(std::string_view a_name, std::shared_ptr<InputButtonBase> a_spButton)
	{
		m_spButtonMap[a_name.data()] = a_spButton;
	}
	void InputCollector::AddAxis(std::string_view a_name, InputAxisBase* a_pAxis)
	{
		AddAxis(a_name.data(),std::shared_ptr<InputAxisBase>(a_pAxis));
	}
	void InputCollector::AddAxis(std::string_view a_name, std::shared_ptr<InputAxisBase> a_spAxis)
	{
		m_spAxisMap[a_name.data()] = a_spAxis;
	}

	// 取得
	const std::shared_ptr<InputButtonBase> InputCollector::GetButton(std::string_view a_name) const
	{
		auto _buttonIt = m_spButtonMap.find(a_name.data());
		if (_buttonIt == m_spButtonMap.end())
		{
			return nullptr;
		}
		return _buttonIt->second;
	}
	const std::shared_ptr<InputAxisBase> InputCollector::GetAxis(std::string_view a_name) const
	{
		auto _axisIt = m_spAxisMap.find(a_name.data());

		if (_axisIt == m_spAxisMap.end())
		{
			return nullptr;
		}
		return _axisIt->second;
	}
	void InputCollector::Release()
	{
		m_spButtonMap.clear();
		m_spAxisMap.clear();
	}
}