#include "ModelEdit.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// 行列を読み取り専用で表示
		//-----------------------------------------------------------------------------------------
		void DrawMatrixText(const char* a_label, const DirectX::XMFLOAT4X4& a_mat)
		{
			if (!ImGui::TreeNode(a_label)) { return; }

			for (int _row = 0; _row < 4; ++_row)
			{
				ImGui::Text(
					"%8.3f, %8.3f, %8.3f, %8.3f",
					a_mat.m[_row][0], a_mat.m[_row][1], a_mat.m[_row][2], a_mat.m[_row][3]
				);
			}
			ImGui::TreePop();
		}

		//-----------------------------------------------------------------------------------------
		// ノード1つ分の詳細表示
		//-----------------------------------------------------------------------------------------
		void DrawNodeDetail(const Resource::Node& a_node, int a_nodeIdx)
		{
			ImGui::Text("Index      : %d", a_nodeIdx);
			ImGui::Text("NameHash   : %u", a_node.nodeNameHash);
			ImGui::Text("Parent     : %d", a_node.parent);
			ImGui::Text("Children   : %zu", a_node.children.size());
			ImGui::Text("BoneIndex  : %d", a_node.boneIndex);
			ImGui::Text("IsSkinMesh : %s", a_node.isSkinMesh ? "true" : "false");

			// このノードが持つメッシュ
			if (a_node.meshIndices.empty())
			{
				ImGui::Text("MeshIndices: none");
			}
			else
			{
				std::string _meshIdxStr;
				for (auto _meshIdx : a_node.meshIndices)
				{
					if (!_meshIdxStr.empty()) { _meshIdxStr += ", "; }
					_meshIdxStr += std::to_string(_meshIdx);
				}
				ImGui::Text("MeshIndices: %s", _meshIdxStr.c_str());
			}

			// 各種行列
			DrawMatrixText("LocalTransform", a_node.localTransform);
			DrawMatrixText("WorldTransform", a_node.worldTransform);
			DrawMatrixText("BoneInverseWorldMatrix", a_node.boneInverseWorldMatrix);
		}

		//-----------------------------------------------------------------------------------------
		// ノード階層を再帰的に表示
		//-----------------------------------------------------------------------------------------
		void DrawNodeTree(const std::vector<Resource::Node>& a_nodeVec, int a_nodeIdx)
		{
			// 不正なインデックスは無視
			if (a_nodeIdx < 0 || a_nodeIdx >= static_cast<int>(a_nodeVec.size())) { return; }

			const auto& _node = a_nodeVec[a_nodeIdx];

			// ノード種別が一目で分かるようにサフィックスを付ける
			std::string _label = _node.name;
			if (_node.boneIndex >= 0) { _label += " [Bone]"; }
			if (!_node.meshIndices.empty()) { _label += " [Mesh]"; }

			// 子を持たないノードも、開けば詳細を見られるようにしておく
			bool _isOpen = ImGui::TreeNodeEx(
				reinterpret_cast<void*>(static_cast<intptr_t>(a_nodeIdx)),
				ImGuiTreeNodeFlags_None,
				"%s", _label.c_str()
			);
			if (!_isOpen) { return; }

			// このノード自身の詳細
			if (ImGui::TreeNode("Detail"))
			{
				DrawNodeDetail(_node, a_nodeIdx);
				ImGui::TreePop();
			}

			// 子ノード
			for (auto _childIdx : _node.children)
			{
				DrawNodeTree(a_nodeVec, _childIdx);
			}

			ImGui::TreePop();
		}
	}

	//-----------------------------------------------------------------------------------------
	// モデルの詳細表示
	//-----------------------------------------------------------------------------------------
	void ModelEdit(EditorContext&, Resource::Model* a_pModel)
	{
		if (!a_pModel) { return; }

		const auto& _assetData = a_pModel->GetAssestData();
		const auto& _nodeVec = a_pModel->GetOriginalNodeVec();

		// ---- 概要 ----
		ImGui::Text("Name      : %s", a_pModel->GetName().c_str());
		ImGui::Text("Nodes     : %zu", _nodeVec.size());
		ImGui::Text("Meshes    : %zu", _assetData.meshGUIDs.size());
		ImGui::Text("Materials : %zu", _assetData.materialGUIDs.size());
		ImGui::Text("Animations: %zu", _assetData.animationGUIDs.size());
		ImGui::Text("Bones     : %zu", a_pModel->GetBoneNodeVec().size());

		ImGui::Separator();

		// ---- ノード階層 ----
		if (ImGui::CollapsingHeader("Node Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextDisabled("(open a node to see its detail)");

			// ルートノードから再帰的に表示
			for (auto _rootIdx : a_pModel->GetRootNodeVec())
			{
				DrawNodeTree(_nodeVec, _rootIdx);
			}
		}

		// ---- アニメーション ----
		if (ImGui::CollapsingHeader("Animations"))
		{
			const auto& _animHandleVec = a_pModel->GetAnimationHandles();
			if (_animHandleVec.empty())
			{
				ImGui::TextDisabled("No animation");
			}

			for (size_t _i = 0; _i < _animHandleVec.size(); ++_i)
			{
				ImGui::PushID(static_cast<int>(_i));

				const auto* _pAnim = Resource::ResourceManager::Instance().Ref(_animHandleVec[_i]);

				// 未ロードの場合はGUIDだけ表示
				if (!_pAnim)
				{
					std::string _guidStr = (_i < _assetData.animationGUIDs.size())
						? _assetData.animationGUIDs[_i].String()
						: std::string("unknown");
					ImGui::Text("[%zu] (not loaded) %s", _i, _guidStr.c_str());
					ImGui::PopID();
					continue;
				}

				if (ImGui::TreeNode("AnimNode", "[%zu] %s", _i, _pAnim->name.c_str()))
				{
					if (_i < _assetData.animationGUIDs.size())
					{
						ImGui::Text("GUID      : %s", _assetData.animationGUIDs[_i].String().c_str());
					}
					ImGui::Text("MaxLength : %.3f frame", _pAnim->maxLength);
					ImGui::Text("AnimNodes : %zu", _pAnim->nodes.size());

					// アニメーションが動かすノードとキー数
					if (ImGui::TreeNode("Channels"))
					{
						for (const auto& _animNode : _pAnim->nodes)
						{
							// 対象ノード名を引けるなら名前で表示
							std::string _targetName = "unknown";
							if (_animNode.nodeOffset >= 0 &&
								_animNode.nodeOffset < static_cast<int>(_nodeVec.size()))
							{
								_targetName = _nodeVec[_animNode.nodeOffset].name;
							}

							ImGui::Text(
								"%s (node %d) : T=%zu R=%zu S=%zu",
								_targetName.c_str(),
								_animNode.nodeOffset,
								_animNode.translations.size(),
								_animNode.rotations.size(),
								_animNode.scales.size()
							);
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		// ---- メッシュ ----
		if (ImGui::CollapsingHeader("Meshes"))
		{
			const auto& _meshHandleVec = a_pModel->GetMeshHandles();
			for (size_t _i = 0; _i < _meshHandleVec.size(); ++_i)
			{
				std::string _guidStr = (_i < _assetData.meshGUIDs.size())
					? _assetData.meshGUIDs[_i].String()
					: std::string("unknown");

				const auto* _pMesh = Resource::ResourceManager::Instance().Ref(_meshHandleVec[_i]);
				if (!_pMesh)
				{
					ImGui::Text("[%zu] (not loaded) %s", _i, _guidStr.c_str());
					continue;
				}

				ImGui::PushID(static_cast<int>(_i));
				if (ImGui::TreeNode("MeshNode", "[%zu] %s", _i, _guidStr.c_str()))
				{
					const auto& _metaData = _pMesh->GetMetaData();
					ImGui::Text("Vertices   : %zu", _pMesh->GetVertexVec().size());
					ImGui::Text("Subsets    : %zu", _metaData.subsets.size());
					ImGui::Text("IsSkinMesh : %s", _metaData.isSkinMesh ? "true" : "false");
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		// ---- マテリアル ----
		if (ImGui::CollapsingHeader("Materials"))
		{
			const auto& _materialHandleVec = a_pModel->GetMaterialHandles();
			for (size_t _i = 0; _i < _materialHandleVec.size(); ++_i)
			{
				std::string _guidStr = (_i < _assetData.materialGUIDs.size())
					? _assetData.materialGUIDs[_i].String()
					: std::string("unknown");

				const auto* _pMaterial = Resource::ResourceManager::Instance().Ref(_materialHandleVec[_i]);
				if (!_pMaterial)
				{
					ImGui::Text("[%zu] (not loaded) %s", _i, _guidStr.c_str());
					continue;
				}

				ImGui::Text("[%zu] %s", _i, _pMaterial->name.c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", _guidStr.c_str());
			}
		}

		// ---- 描画コマンド ----
		if (ImGui::CollapsingHeader("Draw Commands"))
		{
			const auto& _drawCommandVec = a_pModel->GetDrawCommandVec();
			ImGui::Text("Count : %zu", _drawCommandVec.size());

			for (size_t _i = 0; _i < _drawCommandVec.size(); ++_i)
			{
				const auto& _cmd = _drawCommandVec[_i];

				// ノード名が引けるなら名前で表示
				std::string _nodeName = "unknown";
				if (_cmd.nodeIndex < _nodeVec.size())
				{
					_nodeName = _nodeVec[_cmd.nodeIndex].name;
				}

				ImGui::Text(
					"[%zu] node=%s sub=%u meshRawID=%u materialRawID=%u alpha=%s",
					_i,
					_nodeName.c_str(),
					static_cast<UINT>(_cmd.subIdx),
					static_cast<UINT>(_cmd.meshRawID),
					static_cast<UINT>(_cmd.materialRawID),
					std::string(magic_enum::enum_name(_cmd.alphaMode)).c_str()
				);
			}
		}
	}
}
