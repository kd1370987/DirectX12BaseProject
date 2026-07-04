#include "ECSView.h"

#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/ECS/World/World.h"
#include "../../MainEngine.h"
#include "../../Graphics/GraphicEngine.h"

#include "ComponentEdit/ComponentEdit.h"

#include "../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../Application/Components/Persistence/NameComponent.h"
#include "../../../Application/Components/Hierarchy/HierarchyComponent.h"
#include "../../../Application/Components/Persistence/GUIDComponent.h"
namespace Engine::Editor
{
	void ECSView::Init()
	{
	
	}

	void ECSView::Draw(UINT a_widht, UINT a_height)
	{
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		// ヒエラルキー
		HierarchyWindow(_pWorld);

		// インスペクター
		InspectorWindow(_pWorld);

		m_systemView.Draw(a_widht,a_height);
	}

	void ECSView::HierarchyWindow(Engine::ECS::World* a_pWorld)
	{
		// ウィンドウ開始
		if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_MenuBar))
		{
			// フィルター
			EntityFilter();

			// 全エンティティ取得
			UINT _aliveEntityCount = a_pWorld->GetAliveEntityCount();
			ImGui::Text("EntityNum : %d", _aliveEntityCount);

			// 横線
			ImGui::Separator();

			// エンティティ一覧表示
			DrawEntityList(a_pWorld);
		}
		ImGui::End();
	}

	void ECSView::EntityFilter()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Filter"))
			{
				auto FilterItem = [this](const char* label, EFilterType type) {
					if (ImGui::MenuItem(label, nullptr, m_filterType == type))
					{
						m_filterType = type;
					}
				};

				FilterItem("None", EFilterType::None);
				FilterItem("Player", EFilterType::Player);
				FilterItem("Camera", EFilterType::Camera);
				FilterItem("UI", EFilterType::UI);
				FilterItem("Ground", EFilterType::Ground);

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void ECSView::DrawEntityList(Engine::ECS::World* a_pWorld)
	{
		// エンティティ一覧表示
		ImGui::BeginChild("EntityList");
		{
			// 空のエンティティの追加
			if (ImGui::Button("AddEntity"))
			{
				AddEntity(a_pWorld);
			}
			ImGui::Separator();

			// ループ中のコンポーネント追加/削除によるベクター破損を防ぐため、一度EntityIDのリストをコピーする
			const auto& _entityLocationList = a_pWorld->GetEntityList();
			std::vector<Engine::ECS::Entity> _rootEntities;
			_rootEntities.reserve(_entityLocationList.size());

			for (auto& _location : _entityLocationList)
			{
				Engine::ECS::Entity _entity = a_pWorld->GetEntity(_location);
				// 親を持っていないものをルートとして抽出
				if (_entity != ECS::Limits::INVALID_ENTITY)
				{
					// ヒエラルキーコンポーネントを持っているものは親がいるかチェック
					if(a_pWorld->HasComponent<HierarchyComponent>(_entity))
					{
						auto* _pHieComp = a_pWorld->RefData<HierarchyComponent>(_entity);
						if (_pHieComp->parentGUID != Engine::DefaultGUID)
						{
							continue;
						}
					}
					_rootEntities.push_back(_entity);
				}
			}

			// コピーした安全なリストで描画を回す
			for (const auto& _entity : _rootEntities)
			{
				DrawEntity(a_pWorld, _entity);
			}
		}
		ImGui::EndChild();
	}

	void ECSView::DrawEntity(Engine::ECS::World* a_pWorld, const Engine::ECS::Entity& a_entity)
	{

		// エンティティを取得
		if (a_entity == Engine::ECS::Limits::INVALID_ENTITY) return;

		// ツリーノードして表示 : ドラッグアンドドロップ受け入れ
		ImGuiTreeNodeFlags _flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
		if (m_currentEntity == a_entity)
		{
			_flags |= ImGuiTreeNodeFlags_Selected;
		}

		std::vector<ECS::Entity> _children = GetChildEntities(a_pWorld, a_entity);
		if (_children.empty())
		{
			_flags |= ImGuiTreeNodeFlags_Leaf;
		}

		// ラベル作成
		std::string _label = {};
		if (a_pWorld->HasComponent<NameComponent>(a_entity))
		{
			auto* _nameComp = a_pWorld->RefData<NameComponent>(a_entity);
			_label = _nameComp->name;
		}
		if (_label.empty() || (_label == ""))
		{
			_label = std::to_string(a_entity);
		}
		

		// ツリーノード表示
		bool _isNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)a_entity, _flags, _label.c_str());

		// clickで選択
		if (ImGui::IsItemClicked())
		{
			m_currentEntity = a_entity;
		}
		
		// ドラッグアンドドロップの制御
		HandleDragAndDrop(a_pWorld,a_entity,_label);

		// 子エンティティがあれば再起表示
		if (_isNodeOpen)
		{
			for (ECS::Entity& _child : _children)
			{
				DrawEntity(a_pWorld,_child);
			}

			// ツリーノード終了
			ImGui::TreePop();
		}
	}

	void ECSView::HandleDragAndDrop(ECS::World* a_pWorld, const ECS::Entity& a_entity, const std::string& a_label)
	{
		// ドラッグリソースとして設定 : 自分をドラッグする
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("Entity", &a_entity, sizeof(ECS::Entity));
			ImGui::Text("%s", a_label.c_str());
			ImGui::EndDragDropSource();
		}

		// ドロップターゲットの設定 : 自分にドロップされる
		if (ImGui::BeginDragDropTarget())
		{
			// ペイロードが存在するのなら
			if (const ImGuiPayload* _payload = ImGui::AcceptDragDropPayload("Entity"))
			{
				// 子供のエンティティを取得
				ECS::Entity _child = *(ECS::Entity*)_payload->Data;
				// 自分自身や不正なエンティティは親にできない
				if (_child != a_entity && _child != ECS::Limits::INVALID_ENTITY)
				{
					// 親子関係の再構築処理を実行
					AttachChild(a_pWorld, a_entity, _child);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void ECSView::AttachChild(ECS::World* a_pWorld, const ECS::Entity& a_parent, const ECS::Entity& a_child)
	{
		// 安全チェック: 必要なGUIDコンポーネントがあるか
		if (!a_pWorld->HasComponent<GUIDComponent>(a_parent) || !a_pWorld->HasComponent<GUIDComponent>(a_child)) return;

		auto* _pParentGUID = a_pWorld->RefData<GUIDComponent>(a_parent);
		auto* _pChildGUID = a_pWorld->RefData<GUIDComponent>(a_child);

		// すでに子が別の親を持っていた場合は古い親から離脱させる
		//if (a_pWorld->HasComponent<HierarchyComponent>(a_child))
		//{
		//	DettachChild(a_pWorld, a_child);
		//}

		// 新たな親子関係の構築
		Engine::GUID _parentGUID = _pParentGUID->guid;

		UINT _depth = 1;
		// 親がヒエラルキーを持っていたら情報を引き継ぐ
		if (a_pWorld->HasComponent<HierarchyComponent>(a_parent))
		{
			auto* _pHieComp = a_pWorld->RefData<HierarchyComponent>(a_parent);
			// 深度の更新
			_depth = _pHieComp->depth + 1;
		}
		// 持っていなければ親に付与して先に引っ越しさせる
		else
		{
			// コンポーネントの追加
			auto _compTypeID = a_pWorld->GetCompTypeID<HierarchyComponent>();
			HierarchyComponent _initData = {};
			_initData.parentGUID = Engine::DefaultGUID;
			_initData.parentID = ECS::Limits::INVALID_ENTITY;
			_initData.depth = 0;
			// 内部でディープコピーしてるのでローカルでいい
			a_pWorld->AddComponent(_compTypeID, a_parent,(uint8_t*)&_initData);
		}
		// ヒエラルキーコンポーネントを持っていなかった場合
		// 付与してデータを入れる
		HierarchyComponent _newComp = {};
		_newComp.parentGUID = _parentGUID;
		_newComp.parentID = a_parent;
		_newComp.depth = _depth;

		if (a_pWorld->HasComponent<HierarchyComponent>(a_child))
		{
			*a_pWorld->RefData<HierarchyComponent>(a_child) = _newComp;
		}
		else
		{
			auto _compTypeID = a_pWorld->GetCompTypeID<HierarchyComponent>();
			a_pWorld->AddComponent(_compTypeID, a_child, (uint8_t*)&_newComp);
		}
	}

	void ECSView::DettachChild(ECS::World* a_pWorld, const ECS::Entity& a_child)
	{
		// ヒエラルキーを持っていなかったらリターン
		if (!a_pWorld->HasComponent<HierarchyComponent>(a_child)) return;

		// データ取得
		//auto* _pHieComp = a_pWorld->RefData<HierarchyComponent>(a_child);
		//Engine::GUID _preGUID = _pHieComp->prevSiblingGUID;
		//Engine::GUID _nextGUID = _pHieComp->nextSiblingGUID;
		//ECS::Entity _preEntity = FindEntityByGUID(a_pWorld,_preGUID);
		//ECS::Entity _nextEntity = FindEntityByGUID(a_pWorld, _nextGUID);

		//Engine::GUID _firstGUID = _pHieComp->firstChildGUID;
		//if (_firstGUID == Engine::GUID())
		//{
		//	if (auto* _pMyGUIDComp = a_pWorld->RefData<GUIDComponent>(a_child))
		//	{
		//		_firstGUID = _pMyGUIDComp->guid;
		//	}
		//}

		//// 前後の兄弟のリンクを繋ぎ直す
		//if (_preEntity != ECS::Limits::INVALID_ENTITY && a_pWorld->HasComponent<HierarchyComponent>(_preEntity))
		//{
		//	auto* _pComp = a_pWorld->RefData<HierarchyComponent>(_preEntity);
		//	_pComp->nextSiblingGUID = _nextGUID;
		//	_pComp->nextSiblingID = _nextEntity;
		//}
		//if (_nextEntity != ECS::Limits::INVALID_ENTITY && a_pWorld->HasComponent<HierarchyComponent>(_nextEntity))
		//{
		//	auto* _pComp = a_pWorld->RefData<HierarchyComponent>(_nextEntity);
		//	_pComp->prevSiblingGUID = _preGUID;
		//	_pComp->prevSiblingID = _preEntity;
		//	_pComp->firstChildGUID = _firstGUID;
		//}

		// 親なしに戻る
		auto _compTypeID = a_pWorld->GetCompTypeID<HierarchyComponent>();
		a_pWorld->SubmitComponent(_compTypeID, a_child);
	}

	std::vector<ECS::Entity> ECSView::GetChildEntities(Engine::ECS::World* a_pWorld, ECS::Entity a_parent)
	{
		std::vector<ECS::Entity> _children;
		if (!a_pWorld->HasComponent<GUIDComponent>(a_parent)) return _children;

		// 親のGUIDを取得
		Engine::GUID _parentGUID = a_pWorld->RefData<GUIDComponent>(a_parent)->guid;

		a_pWorld->ForEach<HierarchyComponent>(
			[&](ECS::ArchetypeChunk* a_pChunk, UINT a_count, HierarchyComponent* a_hieArray)
			{
				for (UINT _i = 0; _i < a_count; ++_i)
				{
					if (a_hieArray[_i].parentGUID == _parentGUID)
					{
						_children.push_back(a_pChunk->entityData[_i]);
					}
				}
			}
		);

		return _children;
	}

	void ECSView::AddEntity(Engine::ECS::World* a_pWorld)
	{
		
		Engine::ECS::Signature _sig = {};
		_sig.set(a_pWorld->GetCompTypeID(typeid(LocalTransformComponent)));
		a_pWorld->AddEntity(_sig);
	}

	void ECSView::InspectorWindow(Engine::ECS::World* a_pWorld)
	{
		if (ImGui::Begin("Inspector"))
		{
			if (ImGui::Button("RemoveEntity"))
			{
				a_pWorld->RemoveEntity(m_currentEntity);
				m_currentEntity = Engine::ECS::Limits::INVALID_ENTITY;
			}

			if (m_currentEntity == Engine::ECS::Limits::INVALID_ENTITY)
			{
				ImGui::End();
				return;
			}

			ImGui::Text("Entity ID : %d", m_currentEntity);

			// エンティティが持っているコンポーネントを羅列する
			const Engine::ECS::EntityLocation& _location = a_pWorld->GetLocation(m_currentEntity);
			Engine::ECS::Signature _sig = a_pWorld->GetSignature(m_currentEntity);

			for (size_t _typeID = 0; _typeID < _sig.size(); ++_typeID)
			{
				// 持っているコンポーネントのみ表示
				if (_sig.test(_typeID))
				{
					// ツリーノード表示
					auto& _metaData = a_pWorld->GetComponentMetaData(static_cast<Engine::ECS::ComponentTypeID>(_typeID));

					if (ImGui::TreeNodeEx(_metaData.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
					{
						// コンポーネントごとの特殊エディター処理を入れる
						auto _func = a_pWorld->GetCompFunc(_typeID).edit;
						if (_func)
						{
							_func(a_pWorld->NRefData(m_currentEntity,_typeID));
						}

						// コンポーネントを削除するボタン
						SubmitCommponent(a_pWorld,_typeID);

						ImGui::TreePop();
					}

				}
			}

			// エンティティに対してコンポーネントを増やす
			AddComponent(a_pWorld);

		}
		ImGui::End();
	}
	void ECSView::AddComponent(Engine::ECS::World* a_pWorld)
	{
		if (ImGui::BeginCombo("Add Component", "Select..."))
		{
			const ECS::Signature& _sig = a_pWorld->GetSignature(m_currentEntity);
			for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
			{
				// 所持していたら表示しない
				if (_sig.test(_typeID)) continue;

				// メタ情報から名前表示
				if (ImGui::Selectable(_meta.name.c_str()))
				{
					// コンポーネントの追加
					a_pWorld->AddComponent(_typeID,m_currentEntity);
				}
			}

			ImGui::EndCombo();
		}
	}
	void ECSView::SubmitCommponent(Engine::ECS::World* a_pWorld, ECS::ComponentTypeID a_typeID)
	{
		if (ImGui::Button("RemoveComponnet"))
		{
			a_pWorld->SubmitComponent(a_typeID, m_currentEntity);
		}
	}
	ECS::Entity ECSView::FindEntityByGUID(Engine::ECS::World* a_pWorld, const Engine::GUID& a_guid)
	{
		if (a_guid == Engine::GUID()) return ECS::Limits::INVALID_ENTITY;

		ECS::Entity _foundEntity = ECS::Limits::INVALID_ENTITY;
		a_pWorld->ForEach<GUIDComponent>(
			[&a_guid, &_foundEntity](ECS::ArchetypeChunk* a_pChunk, UINT a_count, GUIDComponent* a_guidArray)
			{
				// 見つかってたらスキップ
				if (_foundEntity != ECS::Limits::INVALID_ENTITY) return;

				// 検索
				for (UINT _i = 0; _i < a_count; ++_i)
				{
					if (a_guid == a_guidArray[_i].guid)
					{
						_foundEntity = a_pChunk->entityData[_i];
						return;
					}
				}
			}
		);
		return _foundEntity;
	}
}