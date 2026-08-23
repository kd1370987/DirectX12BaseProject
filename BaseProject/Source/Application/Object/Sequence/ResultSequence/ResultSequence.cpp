#include "ResultSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../UI/UIButton/UIButton.h"
#include "../../../Game/GameManager/GameManager.h"

//==========================================================================================
// ResultSequence
//
// リザルト画面で「ホームのボタンが押されたらタイトルへ戻す」ところだけを持つ。
// タイトル(TitleSequence)とまったく同じ作りで、行き先が違うだけ。
//
// ・ボタンは GUID で引く
//     ポインタを持つとシーンの読み込み順に縛られるうえ、保存もできない。
// ・カーソルの中央固定を切る
//     ゲーム中は視点操作のためにカーソルを毎フレーム画面中央へ戻している。
//     そのままではボタンを狙えないので、リザルトの間だけ切って、抜けるときに戻す。
// ・記録(スコア・タイム)はここでは消さない
//     消すのはゲームシーンの入り口(SceneSequence)。ここで消すと、
//     リザルトを見ている間に数字が 0 になってしまう。
//==========================================================================================
namespace App::Object
{
	void ResultSequence::Update(Engine::GameObject::ObjectContext& a_context)
	{
		//==============================================================
		// カーソルの固定を切る
		//--------------------------------------------------------------
		// 毎フレーム呼んでいるのは、他所で固定を入れ直されても
		// リザルトの間は必ず解除された状態にしたいため
		//==============================================================
		if (m_isReleaseCursorLock && a_context.pServices && a_context.pServices->pInputManager)
		{
			if (a_context.pServices->pInputManager->IsCursorLockActive())
			{
				a_context.pServices->pInputManager->SetCursorCentered(false);
			}
		}

		// ボタンへの差し込み(済んでいれば何もしない)
		TryBindButton(a_context);

		// BGM : 初回で鳴らし始め、フェードを進める
		m_bgm.Update(a_context);
	}

	//======================================================================================
	// ボタンへ押下時の処理を差し込む
	//======================================================================================
	void ResultSequence::TryBindButton(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isBound) return;
		if (!a_context.pObjectManager) return;
		if (!m_homeButtonGUID.IsValid()) return;

		auto* _pObject = a_context.pObjectManager->FindByGUID(m_homeButtonGUID);
		if (!_pObject) return;

		// 指定されたGUIDがボタン以外だった場合は差し込まない
		auto* _pButton = dynamic_cast<UIButton*>(_pObject);
		if (!_pButton)
		{
			// 設定ミスに気付けるよう一度だけ知らせて、以降は試さない
			ENGINE_WARNING("[ResultSequence] 指定されたGUIDは UIButton ではありません");
			m_isBound = true;
			return;
		}

		// 押されたらタイトルへ。
		// this を掴むが、ボタンは同じシーンに居るので寿命は一緒に尽きる
		_pButton->SetOnClick([this]() { RequestBackToTitle(); });

		m_isBound = true;
	}

	//======================================================================================
	// タイトルへ戻る
	//======================================================================================
	void ResultSequence::RequestBackToTitle()
	{
		// 連打で何度も積まないようにする
		if (m_isSceneRequested) return;
		if (!m_titleSceneGUID.IsValid())
		{
			ENGINE_WARNING("[ResultSequence] タイトルシーンが設定されていません");
			return;
		}

		m_isSceneRequested = true;

		Engine::Scene::SceneManager::Instance().SetNextScene(
			m_titleSceneGUID, Engine::Scene::SceneChangeType::Replace);
	}

	//======================================================================================
	// 解放 : カーソルの固定を設定値へ戻す
	//======================================================================================
	void ResultSequence::Release(Engine::GameObject::ObjectContext& a_context)
	{
		// 借りているBGMを返す
		m_bgm.Release(a_context);

		if (!m_isReleaseCursorLock) return;
		if (!a_context.pServices) return;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return;

		// リザルトで切ったぶんを戻す。切りっぱなしにすると、
		// 遷移先で視点操作のカーソル固定が効かなくなる
		const bool _isLocked =
			a_context.pServices->pOptionManager->GetInputOption().isCursorLockedToCenter;

		a_context.pServices->pInputManager->SetCursorCentered(_isLocked);
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void ResultSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		a_ar.GUIDField("HomeButtonGUID", m_homeButtonGUID);
		a_ar.GUIDField("TitleSceneGUID", m_titleSceneGUID);
		a_ar.Field("IsReleaseCursorLock", m_isReleaseCursorLock);

		m_bgm.Archive(a_ar);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void ResultSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("Home Button");

		// 同じシーンに置いた UIButton から選ぶ
		std::string _current = "None";
		if (m_homeButtonGUID.IsValid()) _current = m_homeButtonGUID.String();

		if (ImGui::BeginCombo("Button", _current.c_str()))
		{
			if (ImGui::Selectable("None", !m_homeButtonGUID.IsValid()))
			{
				m_homeButtonGUID = {};
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

					const bool _isSelected = (m_homeButtonGUID == _pButton->GetGUID());
					if (ImGui::Selectable(_pButton->GetGUID().String().c_str(), _isSelected))
					{
						m_homeButtonGUID = _pButton->GetGUID();

						// 差し込み直させる
						m_isBound = false;
					}
					if (_isSelected) ImGui::SetItemDefaultFocus();

					ImGui::PopID();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SeparatorText("Title Scene");

		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Scene", "Scene", m_titleSceneGUID);

		m_bgm.DrawInspector(a_context);

		ImGui::SeparatorText("Cursor");

		ImGui::Checkbox("ReleaseCursorLock", &m_isReleaseCursorLock);
		ImGui::TextDisabled("リザルトの間はカーソルの中央固定を切る");

		//----------------------------------------------------------------------
		// 持ち越されてきた記録(表示のみ)
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Carried Data");

		const auto& _gameData = App::Game::GameManager::Instance().GetGameData();

		const char* _resultName = "None";
		switch (_gameData.result)
		{
		case App::Game::EGameResult::Clear:    _resultName = "Clear";    break;
		case App::Game::EGameResult::GameOver: _resultName = "GameOver"; break;
		default: break;
		}

		ImGui::Text("Result : %s", _resultName);
		ImGui::Text("Score  : %d", _gameData.score);
		ImGui::Text("Kill   : %d", _gameData.killCount);
		ImGui::Text("Time   : %.2f", _gameData.time);
		ImGui::Text("Wave   : %d / %d", _gameData.clearedWaveCount, _gameData.totalWaveCount);
		ImGui::TextDisabled("数字を画面に出すのは ScoreHUD の仕事");

		// 実行中の状態は表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Bound     : %s", m_isBound ? "yes" : "no");
		ImGui::Text("Requested : %s", m_isSceneRequested ? "yes" : "no");
	}
}
