#include "GameObjectHierarchyPanel.h"

#include "../../../Scene/SceneManager/SceneManager.h"
#include "../../../GameObject/GameObjectManager/GameObjectManager.h"
#include "../../../GameObject/BaseObject/BaseObject.h"
#include "../../../GameObject/ObjectMetaRegistry/ObjectMetaRegistry.h"

#include "../../Helper/EditorHelper.h"

//==========================================================================================
// GameObjectHierarchyPanel
//
// ECS外オブジェクトの一覧。ドラッグ&ドロップで親子関係を組める。
//
// ・親子は**この一覧の並びだけ**に効く
//     座標も回転も表示状態も親から伝わらない。オブジェクトが増えたときに
//     「この確認ボックスは MissionSelect のもの」と見て分かるようにするためのもの。
//     座標が伝わる親子が要るなら ECS の HierarchyComponent を使う。
//
// ・親はポインタではなく GUID で持つ
//     読み込み順に縛られないうえ、そのまま保存できる。
//     親が見つからないもの(消された・型が変わった)は根として出るので、
//     参照が切れても一覧から消えることはない。
//
// ・検索中は平らに並べる
//     階層をたどると、一致した子が閉じた親の中に隠れて見つけられないため。
//==========================================================================================
namespace Engine::Editor
{
	namespace
	{
		// ドラッグ&ドロップで受け渡す種類の名札
		constexpr const char* DRAG_PAYLOAD_NAME = "GameObject";

		// 一覧に出す見出し : 表示名 + GUIDの頭(同じ型が並んだときに見分けるため)
		std::string MakeLabel(const GameObject::BaseObject* a_pObject)
		{
			const std::string _guid = a_pObject->GetGUID().String();

			return std::string(a_pObject->GetEditorName()) + "  [" + _guid.substr(0, 8) + "]";
		}
	}

	void GameObjectHierarchyPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// このタブを見ている間はインスペクターをGameモードにする
		// (既存のHierarchyPanel/AssetDataBasePanelと同じ流儀。表示中のタブがモードを決める)
		a_editContext.eInspectorType = EInspectorType::Game;

		// 現在のシーンのオブジェクトマネージャーを取得
		auto* _pManager = Engine::Scene::SceneManager::Instance().RefGameObjectManager();
		if (!_pManager)
		{
			ImGui::TextDisabled("No GameObjectManager");
			a_editContext.pGameObject = nullptr;
			return;
		}

		// ------------------------------------------------------------------
		// AddObject : クラスメタマネージャーに登録済みのクラスを選んでシーンへ追加する
		// ------------------------------------------------------------------
		if (Engine::Editor::EditorHelper::CreateButton("AddObject"))
		{
			ImGui::OpenPopup("AddObjectPopup");
		}
		if (ImGui::BeginPopup("AddObjectPopup"))
		{
			ImGui::TextDisabled("Select Class");
			ImGui::Separator();

			const auto& _allMeta = GameObject::ObjectMetaRegistry::Instance().GetAllMeta();
			if (_allMeta.empty())
			{
				ImGui::TextDisabled("No registered class");
			}
			else
			{
				// 数が増えると探せなくなるのでクラス名で絞り込めるようにする
				const std::string& _search = EditorHelper::DrawSearchBox();

				// タイプインデックス順に並べて表示(map は順不同のため一旦ソート)
				std::vector<GameObject::ObjectTypeID> _ids;
				_ids.reserve(_allMeta.size());
				for (const auto& [_id, _meta] : _allMeta) _ids.push_back(_id);
				std::sort(_ids.begin(), _ids.end());

				for (GameObject::ObjectTypeID _id : _ids)
				{
					const auto& _meta = _allMeta.at(_id);
					if (!EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

					std::string _label = _meta.name + "##addobj" + std::to_string(_id);
					if (ImGui::Selectable(_label.c_str()))
					{
						// 生成してそのまま選択状態にする
						GameObject::BaseObject* _pNew = _pManager->AddObjectByTypeID(_id);
						if (_pNew)
						{
							a_editContext.pGameObject = _pNew;
							a_editContext.ClearEntitySelection();
						}
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}

		const auto& _objects = _pManager->GetObjects();

		ImGui::Text("ObjectNum : %d", static_cast<int>(_objects.size()));
		ImGui::Separator();

		// 名前でオブジェクトを探す。出しっぱなしの欄なので入力は消さない
		const std::string& _search = EditorHelper::DrawSearchBox("##ObjectSearch", "Search object...", false);

		ImGui::Separator();

		// 選択中ポインタがまだ生きているか検証(破棄やシーン切り替えでダングリング化するのを防ぐ)
		// 絞り込みで一覧から外れただけの相手を「消えた」と誤判定しないよう、検証は絞り込みの前に行う
		bool _selectedStillAlive = false;

		//------------------------------------------------------------------
		// 親子の対応表を作る
		//
		// 親を指しているのは子の側なので、そのままでは親から子をたどれない。
		// 毎フレーム作り直しているのは、オブジェクトの増減や付け替えを
		// 別に通知してもらわなくて済むようにするため(数十件なら十分安い)
		//------------------------------------------------------------------
		ChildMap _childMap;
		std::vector<GameObject::BaseObject*> _rootVec;

		for (const auto& _upObject : _objects)
		{
			GameObject::BaseObject* _pObject = _upObject.get();
			if (!_pObject) continue;

			if (_pObject == a_editContext.pGameObject) _selectedStillAlive = true;

			const Engine::GUID& _parentGUID = _pObject->GetParentGUID();

			// 親を指していない、または親が見つからないものは根として出す。
			// 参照が切れたものを隠してしまうと、一覧から消えて触れなくなる
			if (!_parentGUID.IsValid() || _pManager->FindByGUID(_parentGUID) == nullptr)
			{
				_rootVec.push_back(_pObject);
				continue;
			}

			_childMap[_parentGUID].push_back(_pObject);
		}

		ImGui::BeginChild("GameObjectList");
		{
			if (_search.empty())
			{
				// 階層で出す
				for (GameObject::BaseObject* _pRoot : _rootVec)
				{
					DrawObjectNode(a_editContext, _pManager, _pRoot, _childMap, _selectedStillAlive);
				}
			}
			else
			{
				//------------------------------------------------------
				// 検索中は平らに並べる
				//
				// 階層のままだと、一致した子が閉じた親の中に隠れてしまう
				//------------------------------------------------------
				for (const auto& _upObject : _objects)
				{
					GameObject::BaseObject* _pObject = _upObject.get();
					if (!_pObject) continue;
					if (!EditorHelper::IsMatchSearch(_search, _pObject->GetEditorName())) continue;

					ImGui::PushID(_pObject);

					const bool _isSelected = (a_editContext.pGameObject == _pObject);
					ImGui::Selectable(MakeLabel(_pObject).c_str(), _isSelected);

					HandleSelect(a_editContext, _pObject, _selectedStillAlive);
					HandleContextMenu(a_editContext, _pObject);

					ImGui::PopID();
				}
			}

			//----------------------------------------------------------
			// 余白へ落とすと根へ戻す
			//
			// 親から外す手段が右クリックメニューだけだと、
			// 深いところから引っ張り出すのに手間がかかる
			//----------------------------------------------------------
			ImVec2 _rest = ImGui::GetContentRegionAvail();
			_rest.y = std::max(_rest.y, ImGui::GetTextLineHeight());
			ImGui::Dummy(_rest);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* _pPayload = ImGui::AcceptDragDropPayload(DRAG_PAYLOAD_NAME))
				{
					const Engine::GUID _childGUID = *static_cast<const Engine::GUID*>(_pPayload->Data);

					if (auto* _pChild = _pManager->FindByGUID(_childGUID))
					{
						_pChild->SetParentGUID({});
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
		ImGui::EndChild();

		// 選択中オブジェクトが一覧に無くなっていたら選択解除
		if (a_editContext.pGameObject && !_selectedStillAlive)
		{
			a_editContext.pGameObject = nullptr;
		}
	}

	//======================================================================================
	// 1件ぶんのノード
	//======================================================================================
	void GameObjectHierarchyPanel::DrawObjectNode(
		EditorContext& a_editContext,
		GameObject::GameObjectManager* a_pManager,
		GameObject::BaseObject* a_pObject,
		const ChildMap& a_childMap,
		bool& a_inoutIsSelectedAlive)
	{
		if (a_pObject == nullptr) return;

		// 子の一覧(持っていなければ空)
		static const std::vector<GameObject::BaseObject*> _EMPTY = {};

		const auto _it = a_childMap.find(a_pObject->GetGUID());
		const std::vector<GameObject::BaseObject*>& _children =
			(_it != a_childMap.end()) ? _it->second : _EMPTY;

		ImGuiTreeNodeFlags _flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanFullWidth |
			ImGuiTreeNodeFlags_DefaultOpen;

		if (a_editContext.pGameObject == a_pObject) _flags |= ImGuiTreeNodeFlags_Selected;

		// 子が居ないものは開けない見た目にする(三角が出ない)
		if (_children.empty()) _flags |= ImGuiTreeNodeFlags_Leaf;

		// ID はポインタで固定する。名前だけだと同じ型が並んだときにぶつかる
		const bool _isOpen = ImGui::TreeNodeEx(a_pObject, _flags, "%s", MakeLabel(a_pObject).c_str());

		// ポップアップは直前の項目の状態を消費するので、必ず最後に回す
		HandleSelect(a_editContext, a_pObject, a_inoutIsSelectedAlive);
		HandleDragAndDrop(a_pManager, a_pObject);
		HandleContextMenu(a_editContext, a_pObject);

		if (!_isOpen) return;

		for (GameObject::BaseObject* _pChild : _children)
		{
			DrawObjectNode(a_editContext, a_pManager, _pChild, a_childMap, a_inoutIsSelectedAlive);
		}

		ImGui::TreePop();
	}

	//======================================================================================
	// 選択
	//======================================================================================
	void GameObjectHierarchyPanel::HandleSelect(
		EditorContext& a_editContext,
		GameObject::BaseObject* a_pObject,
		bool& a_inoutIsSelectedAlive)
	{
		// 折りたたみの三角を押しただけのときは選択を変えない
		if (!ImGui::IsItemClicked() || ImGui::IsItemToggledOpen()) return;

		// ゲームオブジェクト選択に切り替え(ECSエンティティの選択は解除)
		a_editContext.pGameObject = a_pObject;
		a_editContext.ClearEntitySelection();
		a_inoutIsSelectedAlive = true;
	}

	//======================================================================================
	// 右クリックメニュー
	//--------------------------------------------------------------------------------------
	// ポップアップは「直前の項目」の状態を消費するので、
	// 選択判定とドラッグ&ドロップより後に呼ぶこと
	//======================================================================================
	void GameObjectHierarchyPanel::HandleContextMenu(
		EditorContext& a_editContext,
		GameObject::BaseObject* a_pObject)
	{
		if (!ImGui::BeginPopupContextItem()) return;

		if (a_pObject->GetParentGUID().IsValid())
		{
			if (ImGui::MenuItem("Unparent"))
			{
				a_pObject->SetParentGUID({});
			}
			ImGui::Separator();
		}

		if (ImGui::MenuItem("Destroy"))
		{
			a_pObject->RequestDestroy();
			if (a_editContext.pGameObject == a_pObject)
			{
				a_editContext.pGameObject = nullptr;
			}
		}

		ImGui::EndPopup();
	}

	//======================================================================================
	// ドラッグ&ドロップで親を付け替える
	//======================================================================================
	void GameObjectHierarchyPanel::HandleDragAndDrop(
		GameObject::GameObjectManager* a_pManager,
		GameObject::BaseObject* a_pObject)
	{
		// 自分をドラッグする側
		if (ImGui::BeginDragDropSource())
		{
			const Engine::GUID _guid = a_pObject->GetGUID();
			ImGui::SetDragDropPayload(DRAG_PAYLOAD_NAME, &_guid, sizeof(Engine::GUID));

			ImGui::Text("%s", MakeLabel(a_pObject).c_str());
			ImGui::EndDragDropSource();
		}

		// 自分の上に落とされる側
		if (!ImGui::BeginDragDropTarget()) return;

		if (const ImGuiPayload* _pPayload = ImGui::AcceptDragDropPayload(DRAG_PAYLOAD_NAME))
		{
			const Engine::GUID _childGUID = *static_cast<const Engine::GUID*>(_pPayload->Data);

			auto* _pChild = a_pManager->FindByGUID(_childGUID);

			// 自分自身は親にできない。
			// 自分の子孫を親にすると輪ができて、一覧の描画が無限に潜る
			if (_pChild != nullptr &&
				_pChild != a_pObject &&
				!IsDescendantOf(a_pManager, a_pObject, _pChild))
			{
				_pChild->SetParentGUID(a_pObject->GetGUID());
			}
		}

		ImGui::EndDragDropTarget();
	}

	//======================================================================================
	// 輪ができないか調べる
	//--------------------------------------------------------------------------------------
	// 新しい親からたどって子に行き着くなら、その付け替えは輪を作る。
	// 参照が壊れていても止まるよう、たどる回数にも上限を置いてある。
	//======================================================================================
	bool GameObjectHierarchyPanel::IsDescendantOf(
		GameObject::GameObjectManager* a_pManager,
		const GameObject::BaseObject* a_pParent,
		const GameObject::BaseObject* a_pChild)
	{
		if (a_pManager == nullptr || a_pParent == nullptr || a_pChild == nullptr) return false;

		// 万一すでに輪になっていても抜けられるようにする
		constexpr int _DEPTH_LIMIT = 256;

		const GameObject::BaseObject* _pCurrent = a_pParent;

		for (int _i = 0; _i < _DEPTH_LIMIT; ++_i)
		{
			if (_pCurrent == a_pChild) return true;

			const Engine::GUID& _parentGUID = _pCurrent->GetParentGUID();
			if (!_parentGUID.IsValid()) return false;

			_pCurrent = a_pManager->FindByGUID(_parentGUID);
			if (_pCurrent == nullptr) return false;
		}

		return true;
	}
}
