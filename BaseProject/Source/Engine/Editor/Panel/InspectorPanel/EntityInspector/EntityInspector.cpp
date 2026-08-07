#include "EntityInspector.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Scene/BaseScene/BaseScene.h"
#include "../../../../Scene/SceneManager/SceneManager.h"

#include "../../../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../../Application/Components/Persistence/NameComponent.h"
#include "../../../../../Application/Components/Hierarchy/HierarchyComponent.h"
#include "../../../../../Application/Components/Persistence/GUIDComponent.h"

#include "../../../../../Application/Components/Resource/AnimatorComponent.h"
#include "../../../../../Application/Components/Resource/NodePoseComponent.h"
#include "../../../../../Application/Components/Resource/SkeletonPoseComponent.h"

namespace Engine::Editor::Inspector
{
	// コンポーネントの追加
	void AddComponent(EditorContext& a_editContext, Engine::ECS::World* a_pWorld)
	{
		// 指定して追加
		// 編集対象はプライマリ選択(選択リストの先頭)
		const ECS::Entity _entity = a_editContext.GetPrimaryEntity();

		if (ImGui::BeginCombo("Add Component", "Select..."))
		{
			const ECS::Signature& _sig = a_pWorld->GetSignature(_entity);
			for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
			{
				// 所持していたら表示しない
				if (_sig.test(_typeID)) continue;

				// メタ情報から名前表示
				if (ImGui::Selectable(_meta.name.c_str()))
				{
					// コンポーネントの追加
					a_pWorld->AddComponent(_typeID, _entity);
				}
			}

			ImGui::EndCombo();
		}

		// 動きごとに追加
		// アニメーションするエンティティの場合
		if (ImGui::Button("AnimationEntity"))
		{
			ECS::ChangeEntityCmd _cmd = {};
			_cmd.entity = _entity;
			_cmd.toSig = a_pWorld->GetSignature(_entity);
			_cmd.toSig.set(a_pWorld->GetCompTypeID<AnimatorComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<NodePoseComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<SkeletonPoseComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<PostDeserializeTag>());
			if (_cmd.toSig.test(a_pWorld->GetCompTypeID<ActiveTag>()))
			{
				_cmd.toSig.reset(a_pWorld->GetCompTypeID<ActiveTag>());
			}
			a_pWorld->AddChangeSigCommand(_cmd);
		}
	}

	// コンポーネントの削除
	void SubmitCommponent(EditorContext& a_editContext, Engine::ECS::World* a_pWorld, ECS::ComponentTypeID a_typeID)
	{
		ImGui::Separator();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.05f, 0.05f, 1.0f));

		if (ImGui::Button("RemoveComponnet"))
		{
			a_pWorld->SubmitComponent(a_typeID, a_editContext.GetPrimaryEntity());
		}

		ImGui::PopStyleColor(3);
	}

	void EntityInspector(EditorContext& a_editContext)
	{
		// ワールド取得
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		// 削除は選択中のエンティティすべてが対象
		const int _selectedCount = static_cast<int>(a_editContext.selectedEntities.size());
		const std::string _removeLabel = (_selectedCount > 1)
			? ("RemoveEntity (" + std::to_string(_selectedCount) + ")")
			: std::string("RemoveEntity");

		if (ImGui::Button(_removeLabel.c_str()))
		{
			// RemoveEntity は即座にチャンクを詰め替えるため、
			// 選択リストを直接舐めながら消さずに一度コピーしてから回す。
			const std::vector<Engine::ECS::Entity> _removeTargets = a_editContext.selectedEntities;
			for (const Engine::ECS::Entity& _removeEntity : _removeTargets)
			{
				if (_removeEntity == Engine::ECS::Limits::INVALID_ENTITY) continue;
				_pWorld->RemoveEntity(_removeEntity);
			}
			a_editContext.ClearEntitySelection();
		}

		const Engine::ECS::Entity _entity = a_editContext.GetPrimaryEntity();
		if (_entity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			return;
		}

		ImGui::Text("Entity ID : %llu", _entity);

		// 複数選択中は、コンポーネント編集の対象が先頭1体だけであることを明示しておく
		// (移動と削除は選択中すべてに効く)
		if (_selectedCount > 1)
		{
			ImGui::TextDisabled("(%d selected / editing the first)", _selectedCount);
		}

		// エンティティが持っているコンポーネントを羅列する
		const Engine::ECS::EntityLocation& _location = _pWorld->GetLocation(_entity);
		Engine::ECS::Signature _sig = _pWorld->GetSignature(_entity);

		ECS::CompEditContext _compEditContext = {};
		_compEditContext.pWorld = _pWorld;

		for (size_t _typeID = 0; _typeID < _sig.size(); ++_typeID)
		{
			// 持っているコンポーネントのみ表示
			if (_sig.test(_typeID))
			{
				// ツリーノード表示
				auto& _metaData = _pWorld->GetComponentMetaData(static_cast<Engine::ECS::ComponentTypeID>(_typeID));

				if (ImGui::TreeNodeEx(_metaData.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					// コンポーネントごとの特殊エディター処理を入れる
					auto _func = _pWorld->GetCompFunc(_typeID).edit;
					_compEditContext.entity = _entity;
					_compEditContext.pData = _pWorld->NRefData(_entity, _typeID);
					if (_func)
					{
						_func(_compEditContext);
					}

					// コンポーネントを削除するボタン
					SubmitCommponent(a_editContext,_pWorld, _typeID);

					ImGui::TreePop();
				}

			}
		}

		// エンティティに対してコンポーネントを増やす
		AddComponent(a_editContext,_pWorld);
	}
}

