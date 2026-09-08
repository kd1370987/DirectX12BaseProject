#include "InputActionManager.h"

#include "../../UserData/UserData.h"

// エンジン側
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Input/InputCollector/InputCollector.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"

#include "Engine/Editor/Editor.h"

namespace App::Input
{
	namespace
	{
		//==================================================================================
		// 割り当て1件をデバイスへ積む
		//==================================================================================
		void AddBinding(
			Engine::Input::InputCollector& a_collector,
			Game::EGameAction a_action,
			const Game::InputBindingData& a_binding)
		{
			if (const auto* _pAxis = std::get_if<Game::AxisInputData>(&a_binding))
			{
				a_collector.AddAxis(
					a_action,
					std::make_shared<Engine::Input::InputAxisForWindows>(
						_pAxis->up, _pAxis->right, _pAxis->down, _pAxis->left));
			}
			else if (const auto* _pButton = std::get_if<Game::ButtonInputData>(&a_binding))
			{
				a_collector.AddButton(
					a_action,
					std::make_shared<Engine::Input::InputButtonForWindows>(_pButton->key));
			}
		}

		//==================================================================================
		// 割り当てに選べるキーの一覧
		//----------------------------------------------------------------------------------
		// エディターのプルダウンと、今の割り当ての表示に使う。
		// キーコードをそのまま数値で出しても何のキーか分からないため、
		// 「選べるものだけを名前付きで並べる」形にしている。
		//==================================================================================
		const std::vector<std::pair<int, std::string>>& KeyTable()
		{
			static const std::vector<std::pair<int, std::string>> s_keyTable = []
				{
					std::vector<std::pair<int, std::string>> _table;

					// マウスボタン(GetAsyncKeyState で拾えるのでキーと同じ扱い)
					_table.emplace_back(VK_LBUTTON, "Mouse Left");
					_table.emplace_back(VK_RBUTTON, "Mouse Right");
					_table.emplace_back(VK_MBUTTON, "Mouse Middle");

					// 修飾キー・特殊キー
					_table.emplace_back(VK_LSHIFT, "L Shift");
					_table.emplace_back(VK_RSHIFT, "R Shift");
					_table.emplace_back(VK_LCONTROL, "L Ctrl");
					_table.emplace_back(VK_RCONTROL, "R Ctrl");
					_table.emplace_back(VK_LMENU, "L Alt");
					_table.emplace_back(VK_RMENU, "R Alt");
					_table.emplace_back(VK_SPACE, "Space");
					_table.emplace_back(VK_RETURN, "Enter");
					_table.emplace_back(VK_ESCAPE, "Esc");
					_table.emplace_back(VK_TAB, "Tab");
					_table.emplace_back(VK_BACK, "Back Space");
					_table.emplace_back(VK_UP, "Up");
					_table.emplace_back(VK_DOWN, "Down");
					_table.emplace_back(VK_LEFT, "Left");
					_table.emplace_back(VK_RIGHT, "Right");

					// 英字・数字
					for (int _code = 'A'; _code <= 'Z'; ++_code)
					{
						_table.emplace_back(_code, std::string(1, static_cast<char>(_code)));
					}
					for (int _code = '0'; _code <= '9'; ++_code)
					{
						_table.emplace_back(_code, std::string(1, static_cast<char>(_code)));
					}

					// ファンクションキー
					for (int _i = 0; _i < 12; ++_i)
					{
						_table.emplace_back(VK_F1 + _i, "F" + std::to_string(_i + 1));
					}

					return _table;
				}();

			return s_keyTable;
		}

		// キーコードの表示名。一覧に無いものは番号のまま出す
		std::string KeyName(int a_code)
		{
			for (const auto& [_code, _name] : KeyTable())
			{
				if (_code == a_code) return _name;
			}
			return "Unknown(" + std::to_string(a_code) + ")";
		}

		// キーを選ぶプルダウン。選び直されたら true
		bool DrawKeyCombo(const char* a_label, int& a_code)
		{
			bool _isChanged = false;

			if (ImGui::BeginCombo(a_label, KeyName(a_code).c_str()))
			{
				for (const auto& [_code, _name] : KeyTable())
				{
					const bool _isSelected = (_code == a_code);

					if (ImGui::Selectable(_name.c_str(), _isSelected))
					{
						a_code = _code;
						_isChanged = true;
					}
					if (_isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			return _isChanged;
		}

		// 対応表1つ分の編集。どこか触られたら true
		bool DrawActionMapEdit(std::unordered_map<Game::EGameAction, Game::InputBindingData>& a_map)
		{
			bool _isChanged = false;

			// map の並びは毎回変わるので、必ず enum の並びで出す
			for (const auto _action : magic_enum::enum_values<Game::EGameAction>())
			{
				auto _it = a_map.find(_action);
				if (_it == a_map.end()) continue;

				ImGui::PushID(static_cast<int>(_action));
				ImGui::SeparatorText(std::string(magic_enum::enum_name(_action)).c_str());

				if (auto* _pAxis = std::get_if<Game::AxisInputData>(&_it->second))
				{
					_isChanged |= DrawKeyCombo("Up", _pAxis->up);
					_isChanged |= DrawKeyCombo("Right", _pAxis->right);
					_isChanged |= DrawKeyCombo("Down", _pAxis->down);
					_isChanged |= DrawKeyCombo("Left", _pAxis->left);
				}
				else if (auto* _pButton = std::get_if<Game::ButtonInputData>(&_it->second))
				{
					_isChanged |= DrawKeyCombo("Key", _pButton->key);
				}

				ImGui::PopID();
			}

			return _isChanged;
		}
	}

	//======================================================================================
	// 復元
	//======================================================================================
	void InputActionManager::Init(Game::UserData* a_pUserData)
	{
		m_pUserData = a_pUserData;
		if (!m_pUserData) return;

		// 既定は毎回ここで作り直す。ユーザーデータが空でも遊べる状態にするためと、
		// 「既定へ戻す」で戻す先を持っておくため
		m_defaultSettings = Game::MakeDefaultInputSettings();

		// 保存に無いアクションを埋める。
		// 何も保存されていなければ、そのまま既定一式が入る
		ComplementWithDefault(m_pUserData->RefInputSettings());

		Apply();

		// エディター登録
		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[this]()
			{
				if (ImGui::Begin("InputSetting"))
				{
					Edit();
				}
				ImGui::End();
			}
		);
	}

	//======================================================================================
	// 入力デバイスへ反映
	//--------------------------------------------------------------------------------------
	// 束ごと作り直して差し替える。中身を足すだけだと、割り当てを消したときに
	// 古いものが残ってしまう(AddButton/AddAxis は同じアクションの上書きしかしない)。
	//======================================================================================
	void InputActionManager::Apply()
	{
		if (!m_pUserData) return;

		const auto& _settings = m_pUserData->GetInputSettings();
		auto& _inputManager = Engine::Input::InputManager::Instance();

		// ---- キーボード ----
		{
			auto _upKeyboard = std::make_unique<Engine::Input::InputCollector>();

			for (const auto& [_action, _binding] : _settings.keyboard)
			{
				AddBinding(*_upKeyboard, _action, _binding);
			}

			_inputManager.AddDevice(DEVICE_KEYBOARD, std::move(_upKeyboard));
		}

		// ---- マウス ----
		{
			auto _upMouse = std::make_unique<Engine::Input::InputCollector>();

			for (const auto& [_action, _binding] : _settings.mouse)
			{
				AddBinding(*_upMouse, _action, _binding);
			}

			// 視点はマウスの移動量そのものなので、キーの割り当てとは無関係に積む
			_upMouse->AddAxis(
				Game::EGameAction::Look,
				std::make_shared<Engine::Input::InputAxisForWindowsMouse>());

			_inputManager.AddDevice(DEVICE_MOUSE, std::move(_upMouse));
		}
	}

	//======================================================================================
	// 既定へ戻す
	//======================================================================================
	void InputActionManager::ResetToDefault()
	{
		if (!m_pUserData) return;

		auto& _settings = m_pUserData->RefInputSettings();

		_settings.keyboard = m_defaultSettings.keyboard;
		_settings.mouse = m_defaultSettings.mouse;

		ApplyAndSave();

		ENGINE_LOG("入力設定を既定へ戻しました");
	}

	//======================================================================================
	// 保存されていないアクションを既定で埋める
	//======================================================================================
	void InputActionManager::ComplementWithDefault(Game::InputSettings& a_settings) const
	{
		// 既に入っているものは触らない(ユーザーの割り当てを上書きしない)
		for (const auto& [_action, _binding] : m_defaultSettings.keyboard)
		{
			a_settings.keyboard.try_emplace(_action, _binding);
		}
		for (const auto& [_action, _binding] : m_defaultSettings.mouse)
		{
			a_settings.mouse.try_emplace(_action, _binding);
		}
	}

	//======================================================================================
	// 反映して保存
	//======================================================================================
	void InputActionManager::ApplyAndSave()
	{
		if (!m_pUserData) return;

		Apply();
		m_pUserData->Save();
	}

	//======================================================================================
	// エディター
	//--------------------------------------------------------------------------------------
	// 触った先はユーザーデータそのもの。変えた瞬間に入力へ反映し、そのまま保存する
	// (設定画面と同じ振る舞いにしておくと、UI側を作るときにここを移すだけで済む)。
	//======================================================================================
	void InputActionManager::Edit()
	{
		if (!m_pUserData)
		{
			ImGui::TextDisabled("ユーザーデータが設定されていません");
			return;
		}

		auto& _settings = m_pUserData->RefInputSettings();
		bool _isChanged = false;

		// ---- 既定へ戻す ----
		// 保存・リセットは色を付けない(生成=緑 / 削除=赤 の決まりに合わせる)
		if (ImGui::Button("Reset To Default"))
		{
			ResetToDefault();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(作った時の割り当てへ戻す)");

		ImGui::Separator();

		// ---- マウス感度 ----
		// ここは持っているだけで、実際の振り向きの速さは
		// エンジン側の InputOption(Project設定)が持っている
		ImGui::DragFloat("MouseSensitivity", &_settings.mouseSensitivity, 0.01f, 0.01f, 10.0f);
		if (ImGui::IsItemDeactivatedAfterEdit()) _isChanged = true;

		// ---- 割り当て ----
		if (ImGui::CollapsingHeader("Keyboard", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Keyboard");
			_isChanged |= DrawActionMapEdit(_settings.keyboard);
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Mouse"))
		{
			ImGui::TextDisabled("視点(Look)はマウスの移動量そのものなので割り当ては無い");

			ImGui::PushID("Mouse");
			_isChanged |= DrawActionMapEdit(_settings.mouse);
			ImGui::PopID();
		}

		// 触られていたら、その場で入力へ反映してユーザーデータへ書き出す
		if (_isChanged) ApplyAndSave();
	}
}
