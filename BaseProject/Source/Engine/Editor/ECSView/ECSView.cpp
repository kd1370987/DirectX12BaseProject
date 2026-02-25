#include "ECSView.h"

#include "Engine/ECS/World/World.h"

#include "ComponentEdit/ComponentEdit.h"

void ECSView::Init()
{
	m_spCompEdlit = std::make_shared<ComponentEdit>();
	m_spCompEdlit->Init();
}

void ECSView::Draw()
{
	// ヒエラルキー
	HierarchyWindow();

	// インスペクター
	InspectorWindow();
}

std::shared_ptr<ComponentEdit> ECSView::GetCompEdit()
{
	return m_spCompEdlit;
}

void ECSView::HierarchyWindow()
{
	if (!World::Instance().IsInit()) return;

	// ウィンドウ開始
	if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_MenuBar))
	{
		// フィルター

		// 全エンティティ取得
		const std::vector<EntityLocation>& _entityLocationList = World::Instance().GetEntityList();
		UINT _aliveEntityCount = World::Instance().GetAliveEntityCount();
		// エンティティ総数
		ImGui::Text("EntityNum : %d", _aliveEntityCount);

		// 横線
		ImGui::Separator();

		// エンティティ一覧表示
		ImGui::BeginChild("EntityList");
		{
			for (auto& _location : _entityLocationList)
			{
				DrawEntity(_location);
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void ECSView::DrawEntity(const EntityLocation& a_location)
{
	// エンティティを取得
	ECS::Entity _entity = World::Instance().GetEntity(a_location);
	if (_entity == ECS::Limits::INVALID_ENTITY) return;

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


void ECSView::InspectorWindow()
{
	if (ImGui::Begin("Inspector"))
	{
		if (m_currentEntity == ECS::Limits::INVALID_ENTITY)
		{
			ImGui::End();
			return;
		}

		ImGui::Text("%d",m_currentEntity);

		// エンティティが持っているコンポーネントを羅列する
		const EntityLocation& _location = World::Instance().GetLocation(m_currentEntity);
		ECS::Signature _sig = World::Instance().GetSignature(m_currentEntity);

		for (size_t _typeID = 0; _typeID < _sig.size(); ++_typeID)
		{
			// 持っているコンポーネントのみ表示
			if (_sig.test(_typeID))
			{
				// ツリーノード表示
				auto& _metaData = World::Instance().GetComponentMetaData(static_cast<ECS::ComponentTypeID>(_typeID));
				
				if (ImGui::TreeNodeEx(_metaData.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{

					m_spCompEdlit->GetCompEditFunc(_typeID)(m_currentEntity);

					ImGui::TreePop();
				}
				
			}
		}

	}
	ImGui::End();
}
