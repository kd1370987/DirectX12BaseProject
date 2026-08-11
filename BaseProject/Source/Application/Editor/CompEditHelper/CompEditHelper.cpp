#include "CompEditHelper.h"

#include "../../../Engine/ECS/World/World.h"

#include "../../Components/Hierarchy/HierarchyComponent.h"
#include "../../Components/Resource/ModelComponent.h"

namespace App::Editor
{
	void App::Editor::CompEditHelper::SelectParentModelNode(
		Engine::ECS::CompEditContext& a_editContext,
		UINT& a_nodeNameHash, 
		UINT& a_nodeIndex
	)
	{
		auto* _pWorld = a_editContext.pWorld;

		// 親エンティティはヒエラルキーから取得する
		Engine::ECS::Entity _parentID = Engine::ECS::Limits::INVALID_ENTITY;
		if (_pWorld && a_editContext.entity != Engine::ECS::Limits::INVALID_ENTITY)
		{
			auto* _pHierarchy = _pWorld->RefData<HierarchyComponent>(a_editContext.entity);
			if (_pHierarchy) _parentID = _pHierarchy->parentID;
		}

		if (!_pWorld || _parentID == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: No parent via HierarchyComponent.");
			return;
		}

		// 親のモデルコンポーネントを取得
		auto* _pParentModelComp = _pWorld->RefData<ModelComponent>(_parentID);
		if (!_pParentModelComp)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: ModelComponent not found on Parent.");
			return;
		}

		// 描画
		SelectModelNode(
			a_editContext,
			_pParentModelComp->handle,
			a_nodeNameHash,
			a_nodeIndex
		);
	}

	void CompEditHelper::SelectSelfModelNode(Engine::ECS::CompEditContext& a_editContext, UINT& a_nodeNameHash, UINT& a_nodeIndex)
	{
		auto* _pWorld = a_editContext.pWorld;

		// 実体を持たない編集(プレハブのインスペクタ)では entity が無効値で来る。
		// RefData は生きているエンティティ前提で添え字を引くので、必ず先に弾く。
		if (!_pWorld || a_editContext.entity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: No entity. Set the node on the scene entity.");
			return;
		}

		// モデルコンポーネントを取得
		auto* _pSelfModelComp = _pWorld->RefData<ModelComponent>(a_editContext.entity);
		if (!_pSelfModelComp)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: ModelComponent not found on Self.");
			return;
		}

		// 描画
		SelectModelNode(
			a_editContext,
			_pSelfModelComp->handle,
			a_nodeNameHash,
			a_nodeIndex
		);
	}

	void CompEditHelper::SelectModelNode(Engine::ECS::CompEditContext& a_editContext, const Engine::Handle<Engine::Resource::Model>& a_modelHandle, UINT& a_nodeNameHash, UINT& a_nodeIndex)
	{
		// リソースマネージャーから実際のモデルを取得
		const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(a_modelHandle);
		if (!_pParentModel)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
			return;
		}

		// ノード一覧の描画自体はエンジン側の共通ヘルパーに任せる
		Engine::Editor::EditorHelper::DrawModelNodeCombo(
			"Target Node",
			_pParentModel,
			a_nodeIndex,
			a_nodeNameHash
		);
	}

}