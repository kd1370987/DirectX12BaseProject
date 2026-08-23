#include "HomeSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../ObjectPicker.h"
#include "../../UI/UIBase.h"
#include "../../UI/UIButton/UIButton.h"

//==========================================================================================
// HomeSequence
//
// タイトルの次に来る画面。持っているのは「今どこを見ているか」だけ。
//
// ・ボタンはシーンへ置いたものを GUID で引く(TitleSequence と同じ)
//     押されて何をするかをここが差し込むので、ボタン側はホームの知識を持たない。
//
// ・「押したら出すオブジェクト」は GUID の配列
//     一覧の中身は出される側(MissionSelect)が持つ。ここは出し入れするだけ。
//     出す相手の型を知らずに済むよう、BaseObject::SetVisible を通す。
//
// ・切り替えはコールバックからその場で行う
//     頼まれごとを覚えて次の更新で処理する形にすると、押してから1フレーム遅れて出る。
//     コールバックの中では ObjectContext を持てないので、差し込むときに
//     マネージャーを一緒に掴ませてある(シングルトンを名指ししないための経路は保つ)。
//
// ・カーソルの中央固定を切る
//     プレイ中は視点操作のためにカーソルを毎フレーム画面中央へ戻している。
//     そのままではカーソルを動かせず、中央のボタン以外を狙えない。
//==========================================================================================
namespace App::Object
{
	//======================================================================================
	// 更新
	//======================================================================================
	void HomeSequence::Update(Engine::GameObject::ObjectContext& a_context)
	{
		//==============================================================
		// カーソルの固定を切る
		//--------------------------------------------------------------
		// 他所で固定を入れ直されてもホームの間は切れているように、毎フレーム見る
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
	}

	//======================================================================================
	// ボタンへ押下時の処理を差し込む
	//--------------------------------------------------------------------------------------
	// 設定されているのに見つからないものが1つでもあれば、まだ読み込みの途中とみなして
	// 次のフレームへ回す。全部そろってから差し込むので、差し込み漏れが起きない。
	//======================================================================================
	void HomeSequence::TryBindButtons(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isBound) return;

		auto* _pObjectManager = a_context.pObjectManager;
		if (!_pObjectManager) return;
		if (!IsAllReady(_pObjectManager)) return;

		// ボタンは同じシーンに居るので、this もマネージャーも寿命は一緒に尽きる
		if (auto* _pMissionSelect = Picker::Find<UIButton>(_pObjectManager, m_missionSelectButtonGUID))
		{
			_pMissionSelect->SetOnClick(
				[this, _pObjectManager]() { SetMode(EHomeMode::MissionSelect, _pObjectManager); });
		}

		if (auto* _pWarehouse = Picker::Find<UIButton>(_pObjectManager, m_warehouseButtonGUID))
		{
			// 倉庫はまだ無い。押せる状態にされていても、知らせるだけで何もしない
			// (押せるかどうかは ApplyVisible が設定値を見て切り替える)
			_pWarehouse->SetOnClick([]()
				{
					ENGINE_LOG("[HomeSequence] 倉庫は準備中");
				});
		}

		if (auto* _pBack = Picker::Find<UIButton>(_pObjectManager, m_backButtonGUID))
		{
			_pBack->SetOnClick(
				[this, _pObjectManager]() { SetMode(EHomeMode::Home, _pObjectManager); });
		}

		m_isBound = true;

		// そろったところで、今のモードの見た目にしておく
		ApplyVisible(_pObjectManager);
	}

	//======================================================================================
	// 指しているものがそろっているか
	//======================================================================================
	bool HomeSequence::IsAllReady(Engine::GameObject::GameObjectManager* a_pObjectManager) const
	{
		if (!Picker::IsReady<UIButton>(a_pObjectManager, m_missionSelectButtonGUID)) return false;
		if (!Picker::IsReady<UIButton>(a_pObjectManager, m_warehouseButtonGUID))     return false;
		if (!Picker::IsReady<UIButton>(a_pObjectManager, m_backButtonGUID))          return false;

		return true;
	}

	//======================================================================================
	// 見ている場所を切り替える
	//======================================================================================
	void HomeSequence::SetMode(EHomeMode a_mode, Engine::GameObject::GameObjectManager* a_pObjectManager)
	{
		m_mode = a_mode;
		ApplyVisible(a_pObjectManager);
	}

	//======================================================================================
	// 今のモードに合わせて表示を切り替える
	//--------------------------------------------------------------------------------------
	// 消して作り直すのではなく SetVisible で出し入れする。
	// 非表示のUIは描画も入力も止まるので、裏で押せてしまうことがない。
	//======================================================================================
	void HomeSequence::ApplyVisible(Engine::GameObject::GameObjectManager* a_pObjectManager)
	{
		if (a_pObjectManager == nullptr) return;

		const bool _isHome = (m_mode == EHomeMode::Home);

		// トップのボタン
		if (auto* _pUI = Picker::Find<Engine::GameObject::BaseObject>(a_pObjectManager, m_missionSelectButtonGUID))
		{
			_pUI->SetVisible(_isHome);
		}
		if (auto* _pUI = Picker::Find<Engine::GameObject::BaseObject>(a_pObjectManager, m_warehouseButtonGUID))
		{
			_pUI->SetVisible(_isHome);

			// 倉庫の中身はまだ無い。既定では押せない状態(灰色)にしておく
			if (auto* _pButton = dynamic_cast<UIButton*>(_pUI))
			{
				_pButton->SetInteractable(m_isWarehouseInteractable);
			}
		}

		// 戻るボタンはミッションセレクト側
		if (auto* _pUI = Picker::Find<Engine::GameObject::BaseObject>(a_pObjectManager, m_backButtonGUID))
		{
			_pUI->SetVisible(!_isHome);
		}

		// トップでだけ出すもの
		for (const Engine::GUID& _guid : m_homeUIGUIDVec)
		{
			if (auto* _pUI = Picker::Find<Engine::GameObject::BaseObject>(a_pObjectManager, _guid))
			{
				_pUI->SetVisible(_isHome);
			}
		}

		// 押したときに出すもの(MissionSelect など)
		for (const Engine::GUID& _guid : m_missionObjectGUIDVec)
		{
			if (auto* _pObject = Picker::Find<Engine::GameObject::BaseObject>(a_pObjectManager, _guid))
			{
				_pObject->SetVisible(!_isHome);
			}
		}
	}

	//======================================================================================
	// 解放 : カーソルの固定を設定値へ戻す
	//======================================================================================
	void HomeSequence::Release(Engine::GameObject::ObjectContext& a_context)
	{
		if (!m_isReleaseCursorLock) return;
		if (!a_context.pServices) return;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return;

		// ホームで切ったぶんを戻す。切りっぱなしにすると、
		// 出撃先で視点操作のカーソル固定が効かなくなる
		const bool _isLocked =
			a_context.pServices->pOptionManager->GetInputOption().isCursorLockedToCenter;

		a_context.pServices->pInputManager->SetCursorCentered(_isLocked);
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void HomeSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		//----------------------------------------------------------------------
		// ボタン
		//----------------------------------------------------------------------
		a_ar.GUIDField("MissionSelectButtonGUID", m_missionSelectButtonGUID);
		a_ar.GUIDField("WarehouseButtonGUID", m_warehouseButtonGUID);
		a_ar.GUIDField("BackButtonGUID", m_backButtonGUID);

		//----------------------------------------------------------------------
		// 出し分け
		//----------------------------------------------------------------------
		a_ar.GUIDVectorField("HomeUIGUIDs", m_homeUIGUIDVec);
		a_ar.GUIDVectorField("MissionObjectGUIDs", m_missionObjectGUIDVec);

		//----------------------------------------------------------------------
		// ふるまい
		//----------------------------------------------------------------------
		a_ar.Field("IsWarehouseInteractable", m_isWarehouseInteractable);
		a_ar.Field("IsReleaseCursorLock", m_isReleaseCursorLock);

		if (a_ar.IsLoading())
		{
			// 指し先が変わったので差し込み直す。開くのはトップから
			m_isBound = false;
			m_mode = EHomeMode::Home;
		}
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void HomeSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pObjectManager = a_context.pObjectManager;

		//----------------------------------------------------------------------
		// 今どこを見ているか
		//----------------------------------------------------------------------
		// エディターでは押して切り替えられないので、ここから切り替えて配置を見る
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Mode");

		EHomeMode _mode = m_mode;
		if (Engine::Editor::EditorHelper::DrawEnumCombo("Mode", _mode))
		{
			SetMode(_mode, _pObjectManager);
		}
		ImGui::TextDisabled("配置を見るときはここで切り替える(保存はされない)");

		//----------------------------------------------------------------------
		// ボタン
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Buttons");

		if (Picker::DrawCombo<UIButton>("MissionSelect", _pObjectManager, m_missionSelectButtonGUID)) m_isBound = false;
		if (Picker::DrawCombo<UIButton>("Warehouse", _pObjectManager, m_warehouseButtonGUID))         m_isBound = false;
		if (Picker::DrawCombo<UIButton>("Back", _pObjectManager, m_backButtonGUID))                   m_isBound = false;

		if (ImGui::Checkbox("WarehouseInteractable", &m_isWarehouseInteractable))
		{
			// その場で見た目へ反映する
			ApplyVisible(_pObjectManager);
		}
		ImGui::TextDisabled("倉庫はまだ中身が無いので、押しても何も起きない");

		//----------------------------------------------------------------------
		// 出し分け
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Visible Group");

		ImGui::TextDisabled("トップでだけ出すもの(背景・ロゴ・見出しなど)");
		if (Picker::DrawList<Engine::GameObject::BaseObject>("Home Only", _pObjectManager, m_homeUIGUIDVec))
		{
			ApplyVisible(_pObjectManager);
		}

		ImGui::Spacing();

		ImGui::TextDisabled("MissionSelect ボタンを押したときに出すもの");
		if (Picker::DrawList<Engine::GameObject::BaseObject>("Mission Objects", _pObjectManager, m_missionObjectGUIDVec))
		{
			ApplyVisible(_pObjectManager);
		}

		//----------------------------------------------------------------------
		// その他
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Cursor");
		ImGui::Checkbox("ReleaseCursorLock", &m_isReleaseCursorLock);
		ImGui::TextDisabled("ホームの間はカーソルの中央固定を切る");

		// 実行中の状態は表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Bound : %s", m_isBound ? "yes" : "no");
	}
}
