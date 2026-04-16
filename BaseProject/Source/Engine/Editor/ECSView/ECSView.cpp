#include "ECSView.h"

#include "Application/Scene/SceneManager.h"
#include "Engine/ECS/World/World.h"

#include "ComponentEdit/ComponentEdit.h"
namespace Engine::Editor
{
	void ECSView::Init()
	{
		m_spCompEdlit = std::make_shared<ComponentEdit>();
		m_spCompEdlit->Init();
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

	std::shared_ptr<ComponentEdit> ECSView::GetCompEdit()
	{
		return m_spCompEdlit;
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
			ImGui::BeginChild("EntityList");
			{
				for (auto& _location : _entityLocationList)
				{
					DrawEntity(a_pWorld, _location);
				}
			}
			ImGui::EndChild();
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
					m_filterType = m_filterType;
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
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

						m_spCompEdlit->GetCompEditFunc(a_pWorld, _typeID)(m_currentEntity);

						ImGui::TreePop();
					}

				}
			}

		}
		ImGui::End();
	}
}