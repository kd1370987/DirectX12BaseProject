#include "ECSView.h"

#include "Application/Scene/SceneManager.h"
#include "Engine/ECS/World/World.h"

#include "ComponentEdit/ComponentEdit.h"

#include "../../../Application/Components/Transform/TransformComponent.h"
namespace Engine::Editor
{
	void ECSView::Init()
	{
	
	}

	void ECSView::Draw(UINT a_widht, UINT a_height)
	{
		Engine::ECS::World* _pWorld = SceneManager::Instance().RefWorld();
		if (!_pWorld)return;
		if (!_pWorld->IsInit()) return;

		// ヒエラルキー
		HierarchyWindow(_pWorld);

		// インスペクター
		InspectorWindow(_pWorld);
	}

	void ECSView::HierarchyWindow(Engine::ECS::World* a_pWorld)
	{
		// ウィンドウ開始
		if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_MenuBar))
		{
			// フィルター
			EntityFilter();

			// 全エンティティ取得
			const std::vector<Engine::ECS::EntityLocation>& _entityLocationList = a_pWorld->GetEntityList();
			UINT _aliveEntityCount = a_pWorld->GetAliveEntityCount();
			// エンティティ総数
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
				if (ImGui::MenuItem("None", nullptr, m_filterType == EFilterType::None))
				{
					m_filterType = EFilterType::None;
				}
				if (ImGui::MenuItem("Player", nullptr, m_filterType == EFilterType::Player))
				{
					m_filterType = EFilterType::Player;
				}
				if (ImGui::MenuItem("Camera", nullptr, m_filterType == EFilterType::Camera))
				{
					m_filterType = EFilterType::Camera;
				}
				if (ImGui::MenuItem("UI", nullptr, m_filterType == EFilterType::UI))
				{
					m_filterType = EFilterType::UI;
				}
				if (ImGui::MenuItem("Ground", nullptr, m_filterType == EFilterType::Ground))
				{
					m_filterType = EFilterType::Ground;
				}

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
			//if (ImGui::BeginPopupContextItem("EntityPopup"))
			{
				// 空のエンティティの追加
				if (ImGui::Button("AddEntity"))
				{
					AddEntity(a_pWorld);
				}
			}

			const std::vector<Engine::ECS::EntityLocation>& _entityLocationList = a_pWorld->GetEntityList();
			for (auto& _location : _entityLocationList)
			{
				//for (auto& [_compID, _data] : _location.pArchetypeChunk->layoutMap)
				//{
				//	switch (m_filterType)
				//	{
				//	case Engine::Editor::ECSView::EFilterType::None:
				//		break;
				//	case Engine::Editor::ECSView::EFilterType::Player:
				//		break;
				//	case Engine::Editor::ECSView::EFilterType::Camera:
				//		break;
				//	case Engine::Editor::ECSView::EFilterType::Ground:
				//		break;
				//	case Engine::Editor::ECSView::EFilterType::UI:
				//		break;
				//	default:
				//		break;
				//	}
				//}

				DrawEntity(a_pWorld, _location);
			}
		}
		ImGui::EndChild();
	}

	void ECSView::DrawEntity(Engine::ECS::World* a_pWorld, const Engine::ECS::EntityLocation& a_location)
	{
		// エンティティを取得
		Engine::ECS::Entity _entity = a_pWorld->GetEntity(a_location);
		if (_entity == Engine::ECS::Limits::INVALID_ENTITY) return;

		// ツリーノード設定
		ImGuiTreeNodeFlags _flags = ImGuiBackendFlags_None;
		if (m_currentEntity == _entity)
		{
			_flags |= ImGuiTreeNodeFlags_Selected;
		}

		// ラベル作成
		std::string _label = std::to_string(_entity);

		// ツリーノード表示
		bool _isNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)_entity, _flags, _label.c_str());

		if (ImGui::IsItemClicked())
		{
			m_currentEntity = _entity;
		}

		// ツリーノード終了
		if (_isNodeOpen)
		{
			ImGui::TreePop();
		}
	}

	void ECSView::AddEntity(Engine::ECS::World* a_pWorld)
	{
		//空のエンティティ追加
		//if (ImGui::Selectable("AddGameObject"))
		{
			Engine::ECS::Signature _sig = {};
			_sig.set(a_pWorld->GetCompTypeID(typeid(TransformComponent)));

			a_pWorld->AddEntity(_sig);
		}
	}

	void ECSView::InspectorWindow(Engine::ECS::World* a_pWorld)
	{
		if (ImGui::Begin("Inspector"))
		{
			if (m_currentEntity == Engine::ECS::Limits::INVALID_ENTITY)
			{
				ImGui::End();
				return;
			}

			ImGui::Text("%d", m_currentEntity);

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

						//m_spCompEdlit->GetCompEditFunc(a_pWorld, _typeID)(m_currentEntity);

						auto _func = a_pWorld->GetCompFunc(_typeID).edit;
						if (_func)
						{
							_func(a_pWorld->NRefData(m_currentEntity,_typeID));
						}

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
}