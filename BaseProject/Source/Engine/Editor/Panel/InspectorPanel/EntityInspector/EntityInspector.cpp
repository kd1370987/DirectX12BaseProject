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
		if (ImGui::BeginCombo("Add Component", "Select..."))
		{
			const ECS::Signature& _sig = a_pWorld->GetSignature(a_editContext.entity);
			for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
			{
				// 所持していたら表示しない
				if (_sig.test(_typeID)) continue;

				// メタ情報から名前表示
				if (ImGui::Selectable(_meta.name.c_str()))
				{
					// コンポーネントの追加
					a_pWorld->AddComponent(_typeID, a_editContext.entity);
				}
			}

			ImGui::EndCombo();
		}

		// 動きごとに追加
		// アニメーションするエンティティの場合
		if (ImGui::Button("AnimationEntity"))
		{
			ECS::ChangeEntityCmd _cmd = {};
			_cmd.entity = a_editContext.entity;
			_cmd.toSig = a_pWorld->GetSignature(a_editContext.entity);
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
		if (ImGui::Button("RemoveComponnet"))
		{
			a_pWorld->SubmitComponent(a_typeID, a_editContext.entity);
		}
	}

	void EntityInspector(EditorContext& a_editContext)
	{
		// ワールド取得
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		if (ImGui::Button("RemoveEntity"))
		{
			_pWorld->RemoveEntity(a_editContext.entity);
			a_editContext.entity = Engine::ECS::Limits::INVALID_ENTITY;
		}

		if (a_editContext.entity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			return;
		}

		ImGui::Text("Entity ID : %d", a_editContext.entity);

		// エンティティが持っているコンポーネントを羅列する
		const Engine::ECS::EntityLocation& _location = _pWorld->GetLocation(a_editContext.entity);
		Engine::ECS::Signature _sig = _pWorld->GetSignature(a_editContext.entity);

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
					_compEditContext.entity = a_editContext.entity;
					_compEditContext.pData = _pWorld->NRefData(a_editContext.entity, _typeID);
					if (_func)
					{
						//_func(_pWorld->NRefData(a_editContext.entity, _typeID));
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

