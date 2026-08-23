#include "PauseSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../UI/UIButton/UIButton.h"

//==========================================================================================
// PauseSequence
//
// ポーズ画面で「閉じる」「やめて別のシーンへ移る」だけを持つ。
//
// ポーズ画面はゲームのシーンへ重ねて出している(Push)ので、閉じる(Pop)と後ろの
// ゲームがそのまま続きから動き出す。敵の配置もウェーブの進行も残ったままになる。
//
// ・重ねるのはゲーム側(SceneSequence)、閉じるのはここ
//     どちらも同じ入力(Pause)を見るが、更新されるのは一番上のシーンだけなので、
//     開いている間はここ、閉じている間はあちら、と自然に分かれる。
//
// ・やめるときは先に閉じる
//     重ねたままシーンを差し替えると、入れ替わるのは後ろのゲームの方で、
//     ポーズ画面が乗りっぱなしになる。Pop してから Replace を積む。
//     切り替え命令は積んだ順に処理されるので、この2つは同じフレームで片付く。
//==========================================================================================
namespace App::Object
{
	void PauseSequence::Update(Engine::GameObject::ObjectContext& a_context)
	{
		//==============================================================
		// カーソルの固定を切る
		//--------------------------------------------------------------
		// プレイ中は視点操作のためにカーソルを毎フレーム画面中央へ戻している。
		// そのままではカーソルを動かせず、中央のボタン以外を狙えない
		//==============================================================
		if (m_isReleaseCursorLock && a_context.pServices && a_context.pServices->pInputManager)
		{
			if (a_context.pServices->pInputManager->IsCursorLockActive())
			{
				a_context.pServices->pInputManager->SetCursorCentered(false);
			}
		}

		// ボタンへの差し込み(済んでいれば何もしない)
		TryBindButtons(a_context);

		//==============================================================
		// BGM
		//--------------------------------------------------------------
		// 自分のBGMは絞りの対象から外す。対象のままだと自分で自分を小さくする。
		// 下のゲームBGMは重ねている間ずっと絞っておく : 毎フレーム入れているのは、
		// 別の誰かが戻してしまっても押し切れるようにするため
		//==============================================================
		m_bgm.SetDuckTarget(false);
		SequenceBgm::SetGlobalDuck(m_gameBgmDuck);

		m_bgm.Update(a_context);

		//==============================================================
		// 開いたのと同じキーで閉じる
		//--------------------------------------------------------------
		// 開いたフレームの押下はゲーム側が受け取っており、こちらが動き出す
		// 次のフレームには押しっぱなし(Hold)になっている。
		// 押した瞬間(Press)だけを見ているので、開いた勢いで閉じることはない
		//==============================================================
		if (!m_isClosing && a_context.pServices && a_context.pServices->pInputManager)
		{
			if (a_context.pServices->pInputManager->IsPress(m_pauseActionName))
			{
				RequestResume();
			}
		}
	}

	//======================================================================================
	// ボタンへ押下時の処理を差し込む
	//======================================================================================
	void PauseSequence::TryBindButtons(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isBound) return;
		if (!a_context.pObjectManager) return;

		auto* _pResume = dynamic_cast<UIButton*>(
			m_resumeButtonGUID.IsValid() ? a_context.pObjectManager->FindByGUID(m_resumeButtonGUID) : nullptr);
		auto* _pExit = dynamic_cast<UIButton*>(
			m_exitButtonGUID.IsValid() ? a_context.pObjectManager->FindByGUID(m_exitButtonGUID) : nullptr);

		// 設定されているのに見つからないものがあれば、まだ読み込みの途中とみなして次のフレームへ回す
		if (m_resumeButtonGUID.IsValid() && !_pResume) return;
		if (m_exitButtonGUID.IsValid() && !_pExit) return;

		// ボタンは同じシーンに居るので、this を掴んでも寿命は一緒に尽きる
		if (_pResume) _pResume->SetOnClick([this]() { RequestResume(); });
		if (_pExit)   _pExit->SetOnClick([this]() { RequestExitScene(); });

		m_isBound = true;
	}

	//======================================================================================
	// ポーズを閉じてゲームへ戻る
	//======================================================================================
	void PauseSequence::RequestResume()
	{
		// 連打で何度も積まないようにする
		if (m_isClosing) return;
		m_isClosing = true;

		// 自分を外すだけ。後ろのゲームは残っているので続きから動き出す
		Engine::Scene::SceneManager::Instance().SetNextScene(
			Engine::DefaultGUID, Engine::Scene::SceneChangeType::Pop);
	}

	//======================================================================================
	// ポーズを閉じてから、ゲームのシーンを行き先へ差し替える
	//======================================================================================
	void PauseSequence::RequestExitScene()
	{
		if (m_isClosing) return;

		if (!m_exitSceneGUID.IsValid())
		{
			ENGINE_WARNING("[PauseSequence] やめたときの行き先が設定されていません");
			return;
		}

		m_isClosing = true;

		auto& _sceneManager = Engine::Scene::SceneManager::Instance();

		// 先に自分を外す。重ねたまま差し替えると、入れ替わるのは後ろのゲームの方で
		// ポーズ画面が乗りっぱなしになる
		_sceneManager.SetNextScene(Engine::DefaultGUID, Engine::Scene::SceneChangeType::Pop);

		// 続けて後ろのゲームを行き先へ差し替える(命令は積んだ順に処理される)
		_sceneManager.SetNextScene(m_exitSceneGUID, Engine::Scene::SceneChangeType::Replace);
	}

	//======================================================================================
	// 解放 : カーソルの固定を設定値へ戻す
	//======================================================================================
	void PauseSequence::Release(Engine::GameObject::ObjectContext& a_context)
	{
		// 借りているBGMを返す
		m_bgm.Release(a_context);

		// 絞ったぶんを戻す。戻し忘れると、ゲームへ帰った後も小さいままになる
		SequenceBgm::SetGlobalDuck(1.0f);

		if (!m_isReleaseCursorLock) return;
		if (!a_context.pServices) return;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return;

		// ポーズで切ったぶんを戻す。切りっぱなしにすると、
		// 戻った先で視点操作のカーソル固定が効かなくなる
		const bool _isLocked =
			a_context.pServices->pOptionManager->GetInputOption().isCursorLockedToCenter;

		a_context.pServices->pInputManager->SetCursorCentered(_isLocked);
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void PauseSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		a_ar.GUIDField("ResumeButtonGUID", m_resumeButtonGUID);
		a_ar.GUIDField("ExitButtonGUID", m_exitButtonGUID);
		a_ar.GUIDField("ExitSceneGUID", m_exitSceneGUID);
		a_ar.StringField("PauseActionName", m_pauseActionName);
		a_ar.Field("IsReleaseCursorLock", m_isReleaseCursorLock);
		a_ar.Field("GameBgmDuck", m_gameBgmDuck);

		m_bgm.Archive(a_ar);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void PauseSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("Buttons");

		// 同じシーンに置いた UIButton から選ぶ
		auto _drawButtonCombo = [&](const char* a_label, Engine::GUID& a_inoutGUID)
			{
				std::string _current = a_inoutGUID.IsValid() ? a_inoutGUID.String() : "None";

				if (!ImGui::BeginCombo(a_label, _current.c_str())) return;

				if (ImGui::Selectable("None", !a_inoutGUID.IsValid()))
				{
					a_inoutGUID = {};
					m_isBound = false;
				}

				if (a_context.pObjectManager)
				{
					const auto& _objectVec = a_context.pObjectManager->GetObjects();
					for (size_t _i = 0; _i < _objectVec.size(); ++_i)
					{
						auto* _pButton = dynamic_cast<UIButton*>(_objectVec[_i].get());
						if (!_pButton) continue;

						// 同名でもIDがぶつからないようにする
						ImGui::PushID(static_cast<int>(_i));

						const bool _isSelected = (a_inoutGUID == _pButton->GetGUID());
						if (ImGui::Selectable(_pButton->GetGUID().String().c_str(), _isSelected))
						{
							a_inoutGUID = _pButton->GetGUID();

							// 差し込み直させる
							m_isBound = false;
						}
						if (_isSelected) ImGui::SetItemDefaultFocus();

						ImGui::PopID();
					}
				}
				ImGui::EndCombo();
			};

		_drawButtonCombo("Resume", m_resumeButtonGUID);
		_drawButtonCombo("Exit", m_exitButtonGUID);

		ImGui::SeparatorText("Exit Scene");

		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Scene", "Scene", m_exitSceneGUID);
		ImGui::TextDisabled("やめたときの行き先(ホームなど)");
		if (!m_exitSceneGUID.IsValid())
		{
			ImGui::TextDisabled("(未設定 : Exit を押しても移りません)");
		}

		ImGui::SeparatorText("Input");

		ImGui::InputText("Pause Action", &m_pauseActionName);
		ImGui::TextDisabled("これを押しても閉じる。開くのと同じ名前にしておく");

		m_bgm.DrawInspector(a_context);

		ImGui::DragFloat("GameBgmDuck", &m_gameBgmDuck, 0.01f, 0.0f, 1.0f);
		ImGui::TextDisabled("ポーズ中、下のゲームBGMへ掛ける倍率(1で絞らない)");

		ImGui::SeparatorText("Cursor");

		ImGui::Checkbox("ReleaseCursorLock", &m_isReleaseCursorLock);
		ImGui::TextDisabled("ポーズの間はカーソルの中央固定を切る");

		// 実行中の状態は表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Bound   : %s", m_isBound ? "yes" : "no");
		ImGui::Text("Closing : %s", m_isClosing ? "yes" : "no");
	}
}
