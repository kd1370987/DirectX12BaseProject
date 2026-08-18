#include "TitleSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../UI/UIButton/UIButton.h"

//==========================================================================================
// TitleSequence
//
// タイトル画面で「ボタンが押されたらシーンを切り替える」ところだけを持つ。
//
// UIButton は押されたことを知らせるだけの部品にしてあるので、押されて何をするかは
// 使う側が差し込む。ここがその差し込み役で、ボタン自体にはシーンの知識を持たせない。
// リザルトやポーズも、同じ形で別のオブジェクトを用意すればよい。
//
// ・ボタンは GUID で引く
//     ポインタを持つとシーンの読み込み順に縛られるうえ、保存もできない。
//     GameObjectManager::FindByGUID なら保存した参照をそのまま解決できる。
// ・カーソルの中央固定を切る
//     プレイ中は視点操作のためにカーソルを毎フレーム画面中央へ戻している。
//     そのままではカーソルを動かせず、中央のボタン以外を狙えない。
//     タイトルの間だけ切って、抜けるときに設定値へ戻す。
//==========================================================================================
namespace App::Object
{
	void TitleSequence::Update(Engine::GameObject::ObjectContext& a_context)
	{
		//==============================================================
		// カーソルの固定を切る
		//--------------------------------------------------------------
		// 毎フレーム呼んでいるのは、他所で固定を入れ直されても
		// タイトルの間は必ず解除された状態にしたいため
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
	}

	//======================================================================================
	// ボタンへ押下時の処理を差し込む
	//======================================================================================
	void TitleSequence::TryBindButton(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isBound) return;
		if (!a_context.pObjectManager) return;
		if (!m_playButtonGUID.IsValid()) return;

		auto* _pObject = a_context.pObjectManager->FindByGUID(m_playButtonGUID);
		if (!_pObject) return;

		// 指定されたGUIDがボタン以外だった場合は差し込まない
		auto* _pButton = dynamic_cast<UIButton*>(_pObject);
		if (!_pButton)
		{
			// 設定ミスに気付けるよう一度だけ知らせて、以降は試さない
			ENGINE_WARNING("[TitleSequence] 指定されたGUIDは UIButton ではありません");
			m_isBound = true;
			return;
		}

		// 押されたらシーンを切り替える。
		// this を掴むが、ボタンは同じシーンに居るので寿命は一緒に尽きる
		_pButton->SetOnClick([this]() { RequestChangeScene(); });

		m_isBound = true;
	}

	//======================================================================================
	// 遷移先のシーンを読み込む
	//======================================================================================
	void TitleSequence::RequestChangeScene()
	{
		// 連打で何度も積まないようにする
		if (m_isSceneRequested) return;
		if (!m_nextSceneGUID.IsValid())
		{
			ENGINE_WARNING("[TitleSequence] 遷移先のシーンが設定されていません");
			return;
		}

		m_isSceneRequested = true;

		// シーンの切り替えは SceneManager が持っている。
		// (ObjectContext のサービス群には載っていないので、ここだけ直接触る)
		Engine::Scene::SceneManager::Instance().SetNextScene(
			m_nextSceneGUID, Engine::Scene::SceneChangeType::Replace);
	}

	//======================================================================================
	// 解放 : カーソルの固定を設定値へ戻す
	//======================================================================================
	void TitleSequence::Release(Engine::GameObject::ObjectContext& a_context)
	{
		if (!m_isReleaseCursorLock) return;
		if (!a_context.pServices) return;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return;

		// タイトルで切ったぶんを戻す。切りっぱなしにすると、
		// 遷移先で視点操作のカーソル固定が効かなくなる
		const bool _isLocked =
			a_context.pServices->pOptionManager->GetInputOption().isCursorLockedToCenter;

		a_context.pServices->pInputManager->SetCursorCentered(_isLocked);
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void TitleSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		a_ar.GUIDField("PlayButtonGUID", m_playButtonGUID);
		a_ar.GUIDField("NextSceneGUID", m_nextSceneGUID);
		a_ar.Field("IsReleaseCursorLock", m_isReleaseCursorLock);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void TitleSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("Play Button");

		// 同じシーンに置いた UIButton から選ぶ
		std::string _current = "None";
		if (m_playButtonGUID.IsValid()) _current = m_playButtonGUID.String();

		if (ImGui::BeginCombo("Button", _current.c_str()))
		{
			if (ImGui::Selectable("None", !m_playButtonGUID.IsValid()))
			{
				m_playButtonGUID = {};
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

					const bool _isSelected = (m_playButtonGUID == _pButton->GetGUID());
					if (ImGui::Selectable(_pButton->GetGUID().String().c_str(), _isSelected))
					{
						m_playButtonGUID = _pButton->GetGUID();

						// 差し込み直させる
						m_isBound = false;
					}
					if (_isSelected) ImGui::SetItemDefaultFocus();

					ImGui::PopID();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SeparatorText("Next Scene");

		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Scene", "Scene", m_nextSceneGUID);

		ImGui::SeparatorText("Cursor");

		ImGui::Checkbox("ReleaseCursorLock", &m_isReleaseCursorLock);
		ImGui::TextDisabled("タイトルの間はカーソルの中央固定を切る");

		// 実行中の状態は表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Bound     : %s", m_isBound ? "yes" : "no");
		ImGui::Text("Requested : %s", m_isSceneRequested ? "yes" : "no");
	}
}
