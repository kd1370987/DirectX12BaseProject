#pragma once

#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
#include "Engine/Scene/SceneManager\SceneManager.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Persistence/GUIDComponent.h"
#include "../Resource/ModelComponent.h"
struct ExoskeletonAttachmentComponent
{
	// 追従するエンティティ
	Engine::GUID parentGUID = {};											// シリアライズ用
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// ランタイム用

	// 追従するノードのID
	UINT targetNodeHash = 0;		// ノード名のストリングハッシュ値
	UINT targetNodeIdx = 0;			// ランタイム用ノードインデックス

	// オフセット情報
	DirectX::XMFLOAT3 offsetPosition = { 0,0,0 };
	DirectX::XMFLOAT4 offsetRotation = { 0,0,0,1 };
	DirectX::XMFLOAT3 offsetScale = { 0,0,0 };
};


template<>
struct Engine::ECS::ComponentTraits<ExoskeletonAttachmentComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ExoskeletonAttachmentComponent& _comp = Engine::Editor::GetValue<ExoskeletonAttachmentComponent>(a_pData);
		a_ar.Field("parentGUID",_comp.parentGUID);
		a_ar.Field("targetNodeHash",_comp.targetNodeHash);
		a_ar.Field("offsetPosition",_comp.offsetPosition);
		a_ar.Field("offsetRotation",_comp.offsetRotation);
		a_ar.Field("offsetScale",_comp.offsetScale);
	}

	static void Edit(CompEditContext& a_context)
	{
		ExoskeletonAttachmentComponent& _comp = Engine::Editor::GetValue<ExoskeletonAttachmentComponent>(a_context.pData);
		auto _entity = _comp.parentID;
		ImGui::Text("ParentGUID : %s", _comp.parentGUID.String().c_str());
		ImGui::InputScalar("ParentID", ImGuiDataType_U64, &_entity);

		ImGui::Separator();

		ImGui::Text("TargetBoneIdx : %d", _comp.targetNodeIdx);
		ImGui::Text("TargetNodeHash : %d", _comp.targetNodeHash);


		// エンティティの変更がされたらGUIDを変更
		if (_entity != _comp.parentID)
		{
			auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
			auto _typeID = _pWorld->GetCompTypeID<GUIDComponent>();
			uint8_t* _data = _pWorld->NRefData(_entity, _typeID);
			GUIDComponent& _targetGUIDComp = *reinterpret_cast<GUIDComponent*>(_data);
			_comp.parentGUID = _targetGUIDComp.guid;
			_comp.parentID = _pWorld->GetEntity(_comp.parentGUID);
		}
		auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld(); // 先にWorldのポインタを確保しておくと便利

		// --- 親モデルのノード配列から直接名前を抽出してコンボボックスを表示 ---
		if (_pWorld && _comp.parentID != Engine::ECS::Limits::INVALID_ENTITY)
		{
			// 1. 親のモデルコンポーネントを取得
			auto* _pParentModelComp = _pWorld->RefData<ModelComponent>(_comp.parentID);
			if (_pParentModelComp)
			{
				// 2. リソースマネージャーから実際のモデル（Modelクラス）を取得
				const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(_pParentModelComp->handle);
				if (_pParentModel)
				{
					// 3. モデルが管理する全ノード配列を取得
					const auto& _nodes = _pParentModel->GetOriginalNodeVec();

					// 現在選択されているノード名を表示用として取得
					std::string _currentNodeName = "None / Invalid";
					if (_comp.targetNodeIdx < _nodes.size())
					{
						// ※Nodeクラスのメンバに合わせて _nodes[_comp.targetNodeIdx].m_name もしくは GetName() に書き換えてください
						_currentNodeName = _nodes[_comp.targetNodeIdx].name;
					}

					// 4. ImGuiのコンボボックスで選択可能にする
					if (ImGui::BeginCombo("Target Node", _currentNodeName.c_str()))
					{
						for (size_t _i = 0; _i < _nodes.size(); ++_i)
						{
							bool _isSelected = (_comp.targetNodeIdx == _i);

							// 各ノードの名前を取得して選択肢にする
							std::string _nodeName = _nodes[_i].name; // もしくは _nodes[_i].m_name

							if (ImGui::Selectable(_nodeName.c_str(), _isSelected))
							{
								// 選択されたらインデックスを更新
								_comp.targetNodeHash = _nodes[_i].nodeNameHash;
								_comp.targetNodeIdx = static_cast<UINT>(_i);
							}

							if (_isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
					ImGui::Text("TargetNodeIdx : %d", _comp.targetNodeIdx);
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: ModelComponent not found on Parent.");
				ImGui::Text("TargetNodeIdx : %d", _comp.targetNodeIdx);
			}
		}
		else
		{
			ImGui::Text("TargetNodeIdx : %d (Parent ID is Invalid)", _comp.targetNodeIdx);
		}

		ImGui::Separator();

		ImGui::DragFloat3("OffsetPos", &_comp.offsetPosition.x, 0.1f);
		Engine::Editor::Helper::DragRotationDeg3FromQuaternion(_comp.offsetRotation);
		ImGui::DragFloat3("OffsetScale", &_comp.offsetScale.x, 0.1f);
	}
};