#include "HomeSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../UI/UIBase.h"
#include "../../UI/UIButton/UIButton.h"

//==========================================================================================
// HomeSequence
//
// タイトルの次に来る画面。持っているのは「今どこを見ているか」と
// 「どのステージへ出撃するか」だけ。
//
// ・ボタンはシーンへ置いたものを GUID で引く(TitleSequence と同じ)
//     押されて何をするかをここが差し込むので、ボタン側はホームの知識を持たない。
//
// ・ステージ一覧はここが直接並べて描く
//     ステージは増減するものなので、1件ずつシーンへ置くと数を変えるたびに配置し直しになる。
//     並べ方(開始位置・大きさ・間隔)だけを持たせて、項目は設定した数だけその場で描く。
//     押下判定は UIBase の共通実装を使うので、UIButton と同じ当たり方になる。
//
// ・出し分けは UIBase の Visible
//     トップとステージセレクトで出すものを、消さずに表示だけ切り替える。
//
// ・倉庫はまだ無いので、押しても何もしない(既定では押せない状態にしてある)
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
		// プレイ中は視点操作のためにカーソルを毎フレーム画面中央へ戻している。
		// そのままではカーソルを動かせず、中央のボタン以外を狙えない。
		// 他所で固定を入れ直されても切れているように、毎フレーム見る。
		//==============================================================
		if (m_isReleaseCursorLock && a_context.pServices && a_context.pServices->pInputManager)
		{
			if (a_context.pServices->pInputManager->IsCursorLockActive())
			{
				a_context.pServices->pInputManager->SetCursorCentered(false);
			}
		}

		// 一覧の絵を要求する(済んでいれば何もしない)
		RequestStageTextures(a_context);

		// ボタンへの差し込み(済んでいれば何もしない)
		TryBindButtons(a_context);

		//==============================================================
		// ボタンから頼まれた切り替えを処理する
		//--------------------------------------------------------------
		// コールバックの中では ObjectContext を持てないので、
		// 頼まれたことだけ覚えておいて、ここで切り替える。
		// (押した相手がこの後に更新される場合は次のフレームになるが、
		//  1フレームずれるだけで見た目には出ない)
		//==============================================================
		if (m_isModeRequested)
		{
			m_isModeRequested = false;
			SetMode(m_requestedMode, a_context);
		}

		// ステージ一覧の選択
		UpdateStageList(a_context);
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
		if (!a_context.pObjectManager) return;

		auto* _pStageSelect = dynamic_cast<UIButton*>(FindUI(a_context, m_stageSelectButtonGUID));
		auto* _pWarehouse   = dynamic_cast<UIButton*>(FindUI(a_context, m_warehouseButtonGUID));
		auto* _pSortie      = dynamic_cast<UIButton*>(FindUI(a_context, m_sortieButtonGUID));
		auto* _pBack        = dynamic_cast<UIButton*>(FindUI(a_context, m_backButtonGUID));

		if (m_stageSelectButtonGUID.IsValid() && !_pStageSelect) return;
		if (m_warehouseButtonGUID.IsValid()   && !_pWarehouse)   return;
		if (m_sortieButtonGUID.IsValid()      && !_pSortie)      return;
		if (m_backButtonGUID.IsValid()        && !_pBack)        return;

		// ボタンは同じシーンに居るので、this を掴んでも寿命は一緒に尽きる
		if (_pStageSelect)
		{
			_pStageSelect->SetOnClick([this]() { RequestMode(EHomeMode::StageSelect); });
		}

		if (_pWarehouse)
		{
			// 倉庫はまだ無い。押せる状態にされていても、知らせるだけで何もしない
			// (押せるかどうかは ApplyVisible が設定値を見て切り替える)
			_pWarehouse->SetOnClick([]()
				{
					ENGINE_LOG("[HomeSequence] 倉庫は準備中");
				});
		}

		if (_pSortie)
		{
			_pSortie->SetOnClick([this]() { RequestSortie(); });
		}

		if (_pBack)
		{
			_pBack->SetOnClick([this]() { RequestMode(EHomeMode::Home); });
		}

		m_isBound = true;

		// そろったところで、今のモードの見た目にしておく
		ApplyVisible(a_context);
	}

	//======================================================================================
	// 見ている場所の切り替えを頼む
	//======================================================================================
	void HomeSequence::RequestMode(EHomeMode a_mode)
	{
		m_requestedMode = a_mode;
		m_isModeRequested = true;
	}

	//======================================================================================
	// 見ている場所を切り替える
	//======================================================================================
	void HomeSequence::SetMode(EHomeMode a_mode, Engine::GameObject::ObjectContext& a_context)
	{
		m_mode = a_mode;

		// 前の画面のカーソル状態を持ち込まない
		m_hoverIndex = -1;
		m_pressIndex = -1;

		ApplyVisible(a_context);
	}

	//======================================================================================
	// 今のモードに合わせてUIの表示を切り替える
	//--------------------------------------------------------------------------------------
	// 消して作り直すのではなく Visible で出し入れする。
	// 非表示のUIは描画も入力も止まるので、裏で押せてしまうことがない。
	//======================================================================================
	void HomeSequence::ApplyVisible(Engine::GameObject::ObjectContext& a_context)
	{
		const bool _isHome   = (m_mode == EHomeMode::Home);
		const bool _isSelect = (m_mode == EHomeMode::StageSelect);

		// 決まった4つ
		if (auto* _pUI = FindUI(a_context, m_stageSelectButtonGUID)) _pUI->SetVisible(_isHome);
		if (auto* _pUI = FindUI(a_context, m_warehouseButtonGUID))
		{
			_pUI->SetVisible(_isHome);

			// 倉庫の中身はまだ無い。既定では押せない状態(灰色)にしておく
			if (auto* _pButton = dynamic_cast<UIButton*>(_pUI))
			{
				_pButton->SetInteractable(m_isWarehouseInteractable);
			}
		}
		if (auto* _pUI = FindUI(a_context, m_sortieButtonGUID))      _pUI->SetVisible(_isSelect);
		if (auto* _pUI = FindUI(a_context, m_backButtonGUID))        _pUI->SetVisible(_isSelect);

		// 背景や見出しなど、追加で出し分けたいもの
		for (const auto& _guid : m_homeUIGUIDVec)
		{
			if (auto* _pUI = FindUI(a_context, _guid)) _pUI->SetVisible(_isHome);
		}
		for (const auto& _guid : m_selectUIGUIDVec)
		{
			if (auto* _pUI = FindUI(a_context, _guid)) _pUI->SetVisible(_isSelect);
		}
	}

	//======================================================================================
	// ステージ一覧のカーソル判定と選択
	//--------------------------------------------------------------------------------------
	// カーソルが乗っているものは右へ出すだけ(下見)で、押して初めて選択が移る。
	// 出撃するのは「選んでいるもの」なので、一覧から手を離しても行き先は変わらない。
	//======================================================================================
	void HomeSequence::UpdateStageList(Engine::GameObject::ObjectContext& a_context)
	{
		m_hoverIndex = -1;

		if (m_mode != EHomeMode::StageSelect) return;
		if (m_stageVec.empty())
		{
			m_selectIndex = 0;
			m_pressIndex = -1;
			return;
		}

		// 一覧を減らしたときに範囲外を指したままにならないようにする
		m_selectIndex = std::clamp(m_selectIndex, 0, static_cast<int>(m_stageVec.size()) - 1);

		//--------------------------------------------------------------
		// 出撃できるかどうかを毎フレーム反映する
		// (行き先が入っていないステージを選んでいる間は押させない)
		//--------------------------------------------------------------
		if (auto* _pSortie = dynamic_cast<UIButton*>(FindUI(a_context, m_sortieButtonGUID)))
		{
			_pSortie->SetInteractable(m_stageVec[m_selectIndex].sceneGUID.IsValid());
		}

		if (!a_context.pServices || !a_context.pServices->pInputManager) return;

		auto& _input = *a_context.pServices->pInputManager;

		// エディター操作で選択が動かないようにする(UIButton と同じ扱い)
		if (!_input.IsGameInputEnable())
		{
			m_pressIndex = -1;
			return;
		}

		//--------------------------------------------------------------
		// カーソルが乗っている項目を探す
		//--------------------------------------------------------------
		Math::Vector2 _cursorPos = {};
		if (!UIBase::CalcCursorUIPos(a_context, _cursorPos))
		{
			m_pressIndex = -1;
			return;
		}

		for (int _i = 0; _i < static_cast<int>(m_stageVec.size()); ++_i)
		{
			// 判定は描画と同じ矩形から作る。ズレようがないように、
			// 置く位置も大きさも描画に渡すものをそのまま渡す
			if (UIBase::IsPointInside(_cursorPos, CalcItemPos(_i), m_listItemSize, m_listPivot, 0.0f))
			{
				m_hoverIndex = _i;
				break;
			}
		}

		//--------------------------------------------------------------
		// 押下の進行 : 押し始めと離しが同じ項目のときだけ選ぶ
		//--------------------------------------------------------------
		if (_input.IsPress(m_clickActionName))
		{
			m_pressIndex = m_hoverIndex;
		}

		if (_input.IsRelease(m_clickActionName))
		{
			if (m_pressIndex >= 0 && m_pressIndex == m_hoverIndex)
			{
				m_selectIndex = m_hoverIndex;
			}
			m_pressIndex = -1;
		}
	}

	//======================================================================================
	// 選んでいるステージへ出撃する
	//======================================================================================
	void HomeSequence::RequestSortie()
	{
		// 連打で何度も積まないようにする
		if (m_isSceneRequested) return;

		if (m_selectIndex < 0 || m_selectIndex >= static_cast<int>(m_stageVec.size()))
		{
			ENGINE_WARNING("[HomeSequence] 出撃先のステージが選ばれていません");
			return;
		}

		const Engine::GUID& _sceneGUID = m_stageVec[m_selectIndex].sceneGUID;
		if (!_sceneGUID.IsValid())
		{
			ENGINE_WARNING("[HomeSequence] このステージにはシーンが設定されていません");
			return;
		}

		m_isSceneRequested = true;

		// シーンの切り替えは SceneManager が持っている
		// (ObjectContext のサービス群には載っていないので、ここだけ直接触る)
		Engine::Scene::SceneManager::Instance().SetNextScene(
			_sceneGUID, Engine::Scene::SceneChangeType::Replace);
	}

	//======================================================================================
	// GUID から UI を引く
	//======================================================================================
	UIBase* HomeSequence::FindUI(Engine::GameObject::ObjectContext& a_context, const Engine::GUID& a_guid) const
	{
		if (!a_context.pObjectManager) return nullptr;
		if (!a_guid.IsValid()) return nullptr;

		// UI 以外を指していたら nullptr。設定ミスでも落ちないようにする
		return dynamic_cast<UIBase*>(a_context.pObjectManager->FindByGUID(a_guid));
	}

	//======================================================================================
	// 一覧の絵を読み込む
	//--------------------------------------------------------------------------------------
	// 実体が届くのを待つ必要はないので、要求だけ出して先へ進む。
	// 描画側は届いていないぶんを飛ばす。
	//======================================================================================
	void HomeSequence::RequestStageTextures(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isTexRequested) return;
		if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

		auto& _resource = *a_context.pServices->pResourceManager;

		for (auto& _stage : m_stageVec)
		{
			if (_stage.listTexGUID.IsValid())
			{
				_stage.listTexRef = _resource.RequestLoad<Engine::Resource::Texture>(_stage.listTexGUID);
			}
			if (_stage.imageTexGUID.IsValid())
			{
				_stage.imageTexRef = _resource.RequestLoad<Engine::Resource::Texture>(_stage.imageTexGUID);
			}
			if (_stage.descTexGUID.IsValid())
			{
				_stage.descTexRef = _resource.RequestLoad<Engine::Resource::Texture>(_stage.descTexGUID);
			}
		}

		m_isTexRequested = true;
	}

	//======================================================================================
	// 一覧の a_index 番目を置く位置
	//======================================================================================
	Math::Vector2 HomeSequence::CalcItemPos(int a_index) const
	{
		// 縦へ送る。送り幅は項目の高さ + 間隔
		const float _step = m_listItemSize.y + m_listSpacing;

		return {
			m_listOrigin.x,
			m_listOrigin.y + _step * static_cast<float>(a_index)
		};
	}

	//======================================================================================
	// 右へ出すステージ
	//--------------------------------------------------------------------------------------
	// カーソルが乗っていればその下見、乗っていなければ選んでいるもの。
	//======================================================================================
	const HomeSequence::StageEntry* HomeSequence::GetShowEntry() const
	{
		const int _index = (m_hoverIndex >= 0) ? m_hoverIndex : m_selectIndex;

		if (_index < 0 || _index >= static_cast<int>(m_stageVec.size())) return nullptr;

		return &m_stageVec[_index];
	}

	//======================================================================================
	// 描画 : ステージ一覧と、選んだステージの画像・説明文
	//======================================================================================
	void HomeSequence::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		// トップにはここが描くものは無い(ボタンはそれぞれのUIが描く)
		if (m_mode != EHomeMode::StageSelect) return;

		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		//--------------------------------------------------------------
		// 左 : ステージ一覧
		//--------------------------------------------------------------
		for (int _i = 0; _i < static_cast<int>(m_stageVec.size()); ++_i)
		{
			// 選んでいるものが一番目立ち、次にカーソルが乗っているもの
			const Math::Color* _pColor = &m_listNormalColor;
			if (_i == m_hoverIndex)  _pColor = &m_listHoverColor;
			if (_i == m_selectIndex) _pColor = &m_listSelectColor;

			_pGE->SubmitUI(
				m_stageVec[_i].listTexRef,
				CalcItemPos(_i),
				m_listItemSize,
				*_pColor,
				0.0f,
				m_layer,
				{},
				m_listPivot
			);
		}

		//--------------------------------------------------------------
		// 右 : ステージ画像と説明文
		//--------------------------------------------------------------
		const StageEntry* _pShow = GetShowEntry();
		if (!_pShow) return;

		_pGE->SubmitUI(
			_pShow->imageTexRef,
			m_imagePos,
			m_imageSize,
			Engine::Color::WHITE,
			0.0f,
			m_layer,
			{},
			m_imagePivot
		);

		_pGE->SubmitUI(
			_pShow->descTexRef,
			m_descPos,
			m_descSize,
			Engine::Color::WHITE,
			0.0f,
			m_layer,
			{},
			m_descPivot
		);
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
		a_ar.GUIDField("StageSelectButtonGUID", m_stageSelectButtonGUID);
		a_ar.GUIDField("WarehouseButtonGUID", m_warehouseButtonGUID);
		a_ar.GUIDField("SortieButtonGUID", m_sortieButtonGUID);
		a_ar.GUIDField("BackButtonGUID", m_backButtonGUID);

		a_ar.GUIDVectorField("HomeUIGUIDs", m_homeUIGUIDVec);
		a_ar.GUIDVectorField("SelectUIGUIDs", m_selectUIGUIDVec);

		//----------------------------------------------------------------------
		// ステージ一覧
		//----------------------------------------------------------------------
		size_t _stageCount = m_stageVec.size();
		if (a_ar.BeginArray("Stages", _stageCount))
		{
			m_stageVec.resize(_stageCount);

			for (size_t _i = 0; _i < _stageCount; ++_i)
			{
				if (!a_ar.BeginObject(_i)) continue;

				StageEntry& _stage = m_stageVec[_i];
				a_ar.StringField("name", _stage.name);
				a_ar.GUIDField("listTexGUID", _stage.listTexGUID);
				a_ar.GUIDField("imageTexGUID", _stage.imageTexGUID);
				a_ar.GUIDField("descTexGUID", _stage.descTexGUID);
				a_ar.GUIDField("sceneGUID", _stage.sceneGUID);

				a_ar.EndObject();
			}
			a_ar.EndArray();
		}

		//----------------------------------------------------------------------
		// 並べ方
		//----------------------------------------------------------------------
		a_ar.Field("ListOrigin", m_listOrigin);
		a_ar.Field("ListItemSize", m_listItemSize);
		a_ar.Field("ListPivot", m_listPivot);
		a_ar.Field("ListSpacing", m_listSpacing);

		a_ar.Field("ListNormalColor", m_listNormalColor);
		a_ar.Field("ListHoverColor", m_listHoverColor);
		a_ar.Field("ListSelectColor", m_listSelectColor);

		a_ar.Field("ImagePos", m_imagePos);
		a_ar.Field("ImageSize", m_imageSize);
		a_ar.Field("ImagePivot", m_imagePivot);

		a_ar.Field("DescPos", m_descPos);
		a_ar.Field("DescSize", m_descSize);
		a_ar.Field("DescPivot", m_descPivot);

		a_ar.Field("Layer", m_layer);
		a_ar.StringField("ClickActionName", m_clickActionName);

		//----------------------------------------------------------------------
		// ふるまい
		//----------------------------------------------------------------------
		a_ar.Field("IsWarehouseInteractable", m_isWarehouseInteractable);
		a_ar.Field("IsReleaseCursorLock", m_isReleaseCursorLock);

		// 読み込んだら絵を要求し直す
		if (a_ar.IsLoading())
		{
			m_isTexRequested = false;
		}
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void HomeSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		//----------------------------------------------------------------------
		// 今どこを見ているか
		//----------------------------------------------------------------------
		// エディターでは押して切り替えられないので、ここから切り替えて配置を見る
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Mode");

		EHomeMode _mode = m_mode;
		if (Engine::Editor::EditorHelper::DrawEnumCombo("Mode", _mode))
		{
			SetMode(_mode, a_context);
		}
		ImGui::TextDisabled("配置を見るときはここで切り替える(保存はされない)");

		//----------------------------------------------------------------------
		// ボタン
		//----------------------------------------------------------------------
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

		_drawButtonCombo("StageSelect", m_stageSelectButtonGUID);
		_drawButtonCombo("Warehouse", m_warehouseButtonGUID);
		_drawButtonCombo("Sortie", m_sortieButtonGUID);
		_drawButtonCombo("Back", m_backButtonGUID);

		if (ImGui::Checkbox("WarehouseInteractable", &m_isWarehouseInteractable))
		{
			// その場で見た目へ反映する
			ApplyVisible(a_context);
		}
		ImGui::TextDisabled("倉庫はまだ中身が無いので、押しても何も起きない");

		//----------------------------------------------------------------------
		// 出し分け(背景・見出しなど)
		//----------------------------------------------------------------------
		auto _drawUIList = [&](const char* a_label, std::vector<Engine::GUID>& a_inoutGUIDVec)
			{
				if (!ImGui::TreeNode(a_label)) return;

				int _removeIndex = -1;
				for (size_t _i = 0; _i < a_inoutGUIDVec.size(); ++_i)
				{
					ImGui::PushID(static_cast<int>(_i));

					std::string _current = a_inoutGUIDVec[_i].IsValid() ? a_inoutGUIDVec[_i].String() : "None";
					if (ImGui::BeginCombo("UI", _current.c_str()))
					{
						if (a_context.pObjectManager)
						{
							const auto& _objectVec = a_context.pObjectManager->GetObjects();
							for (size_t _o = 0; _o < _objectVec.size(); ++_o)
							{
								auto* _pUI = dynamic_cast<UIBase*>(_objectVec[_o].get());
								if (!_pUI) continue;

								ImGui::PushID(static_cast<int>(_o));

								const bool _isSelected = (a_inoutGUIDVec[_i] == _pUI->GetGUID());
								if (ImGui::Selectable(_pUI->GetGUID().String().c_str(), _isSelected))
								{
									a_inoutGUIDVec[_i] = _pUI->GetGUID();
								}
								if (_isSelected) ImGui::SetItemDefaultFocus();

								ImGui::PopID();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SameLine();
					if (Engine::Editor::EditorHelper::DeleteSmallButton("X")) _removeIndex = static_cast<int>(_i);

					ImGui::PopID();
				}

				if (_removeIndex >= 0)
				{
					a_inoutGUIDVec.erase(a_inoutGUIDVec.begin() + _removeIndex);
				}

				if (Engine::Editor::EditorHelper::CreateButton("Add UI"))
				{
					a_inoutGUIDVec.push_back({});
				}

				ImGui::TreePop();
			};

		ImGui::SeparatorText("Visible Group");
		ImGui::TextDisabled("上の4つ以外に出し分けたいUI(背景・見出しなど)");
		_drawUIList("Home Only", m_homeUIGUIDVec);
		_drawUIList("StageSelect Only", m_selectUIGUIDVec);

		//----------------------------------------------------------------------
		// ステージ一覧
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Stages");
		ImGui::TextDisabled("文字を出す仕組みが無いので、ステージ名も説明文も画像で用意する");

		int _removeStage = -1;
		for (size_t _i = 0; _i < m_stageVec.size(); ++_i)
		{
			StageEntry& _stage = m_stageVec[_i];

			ImGui::PushID(static_cast<int>(_i));

			const std::string _label = std::to_string(_i) + " : " + _stage.name;
			if (ImGui::TreeNode(_label.c_str()))
			{
				ImGui::InputText("Name", &_stage.name);
				ImGui::TextDisabled("エディターで見分けるためだけの名前");

				// 絵を差し替えたら要求し直す
				bool _isTexChanged = false;
				_isTexChanged |= Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("ListTex", "Texture", _stage.listTexGUID);
				_isTexChanged |= Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("ImageTex", "Texture", _stage.imageTexGUID);
				_isTexChanged |= Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("DescTex", "Texture", _stage.descTexGUID);
				if (_isTexChanged) m_isTexRequested = false;

				Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Scene", "Scene", _stage.sceneGUID);
				ImGui::TextDisabled("出撃ボタンで飛ぶ先");

				// 見た目の確認用
				Engine::Editor::EditorHelper::DrawTexture(_stage.imageTexRef, 192, 108);

				if (Engine::Editor::EditorHelper::DeleteButton("Remove Stage"))
				{
					_removeStage = static_cast<int>(_i);
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (_removeStage >= 0)
		{
			m_stageVec.erase(m_stageVec.begin() + _removeStage);

			// 消したぶんで指し先がずれるので直す
			m_selectIndex = std::clamp(m_selectIndex, 0, std::max(static_cast<int>(m_stageVec.size()) - 1, 0));
			m_hoverIndex = -1;
		}

		if (Engine::Editor::EditorHelper::CreateButton("Add Stage"))
		{
			m_stageVec.push_back({});
			m_isTexRequested = false;
		}

		//----------------------------------------------------------------------
		// 並べ方
		//----------------------------------------------------------------------
		if (ImGui::CollapsingHeader("Layout", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SeparatorText("List (left)");
			ImGui::DragFloat2("ListOrigin", &m_listOrigin.x, 1.0f);
			ImGui::TextDisabled("1番上の項目の位置(px)");
			ImGui::DragFloat2("ListItemSize", &m_listItemSize.x, 1.0f, 0.0f, 8192.0f);
			ImGui::DragFloat2("ListPivot (0-1)", &m_listPivot.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("ListSpacing", &m_listSpacing, 0.5f);
			ImGui::TextDisabled("項目と項目の間隔(px)");

			ImGui::SeparatorText("List Color");
			Engine::Editor::EditorHelper::DrawColorEdit("Normal", m_listNormalColor);
			Engine::Editor::EditorHelper::DrawColorEdit("Hover", m_listHoverColor);
			Engine::Editor::EditorHelper::DrawColorEdit("Select", m_listSelectColor);

			ImGui::SeparatorText("Detail (right)");
			ImGui::DragFloat2("ImagePos", &m_imagePos.x, 1.0f);
			ImGui::DragFloat2("ImageSize", &m_imageSize.x, 1.0f, 0.0f, 8192.0f);
			ImGui::DragFloat2("ImagePivot (0-1)", &m_imagePivot.x, 0.01f, 0.0f, 1.0f);

			ImGui::DragFloat2("DescPos", &m_descPos.x, 1.0f);
			ImGui::DragFloat2("DescSize", &m_descSize.x, 1.0f, 0.0f, 8192.0f);
			ImGui::DragFloat2("DescPivot (0-1)", &m_descPivot.x, 0.01f, 0.0f, 1.0f);

			ImGui::SeparatorText("Common");
			ImGui::DragFloat("LayerZ", &m_layer, 1.0f);
			ImGui::InputText("ClickAction", &m_clickActionName);
			ImGui::TextDisabled("一覧を選ぶ入力。UIButton と揃えておく");
		}

		//----------------------------------------------------------------------
		// その他
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Cursor");
		ImGui::Checkbox("ReleaseCursorLock", &m_isReleaseCursorLock);
		ImGui::TextDisabled("ホームの間はカーソルの中央固定を切る");

		// 実行中の状態は表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Bound     : %s", m_isBound ? "yes" : "no");
		ImGui::Text("Select    : %d", m_selectIndex);
		ImGui::Text("Hover     : %d", m_hoverIndex);
		ImGui::Text("Requested : %s", m_isSceneRequested ? "yes" : "no");
	}
}
