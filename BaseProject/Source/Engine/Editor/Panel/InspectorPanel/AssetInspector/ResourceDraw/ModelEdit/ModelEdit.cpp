#include "ModelEdit.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../AssetLink.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// 行列を読み取り専用で表示
		//-----------------------------------------------------------------------------------------
		void DrawMatrixText(const char* a_label, const Math::Matrix& a_mat)
		{
			if (!ImGui::TreeNode(a_label)) { return; }

			for (int _row = 0; _row < 4; ++_row)
			{
				Engine::Editor::Text("%8.3f, %8.3f, %8.3f, %8.3f", a_mat.m[_row][0], a_mat.m[_row][1], a_mat.m[_row][2], a_mat.m[_row][3]);
			}
			ImGui::TreePop();
		}

		//-----------------------------------------------------------------------------------------
		// ノード1つ分の詳細表示
		//-----------------------------------------------------------------------------------------
		void DrawNodeDetail(const Resource::Node& a_node, int a_nodeIdx)
		{
			Engine::Editor::Value("Index", "%d", a_nodeIdx);
			Engine::Editor::Value("NameHash", "%u", a_node.nodeNameHash);
			Engine::Editor::Value("Parent", "%d", a_node.parent);
			Engine::Editor::Value("Children", "%zu", a_node.children.size());
			Engine::Editor::Value("BoneIndex", "%d", a_node.boneIndex);
			Engine::Editor::Value("IsSkinMesh", "%s", a_node.isSkinMesh ? "true" : "false");

			// このノードが持つメッシュ
			if (a_node.meshIndices.empty())
			{
				Engine::Editor::Value("MeshIndices", "none");
			}
			else
			{
				std::string _meshIdxStr;
				for (auto _meshIdx : a_node.meshIndices)
				{
					if (!_meshIdxStr.empty()) { _meshIdxStr += ", "; }
					_meshIdxStr += std::to_string(_meshIdx);
				}
				Engine::Editor::Value("MeshIndices", "%s", _meshIdxStr.c_str());
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
	void ModelEdit(EditorContext& a_editContext, Resource::Model* a_pModel)
	{
		if (!a_pModel) { return; }

		const auto& _assetData = a_pModel->GetAssestData();
		const auto& _nodeVec = a_pModel->GetOriginalNodeVec();

		// ---- 概要 ----
		Engine::Editor::Value("Name", "%s", a_pModel->GetName().c_str());
		Engine::Editor::Value("Nodes", "%zu", _nodeVec.size());
		Engine::Editor::Value("Meshes", "%zu", _assetData.meshGUIDs.size());
		Engine::Editor::Value("Materials", "%zu", _assetData.materialGUIDs.size());
		Engine::Editor::Value("Animations", "%zu", _assetData.animationGUIDs.size());
		Engine::Editor::Value("Bones", "%zu", a_pModel->GetBoneNodeVec().size());

		Engine::Editor::Line();

		// ---- ノード階層 ----
		if (ImGui::CollapsingHeader("Node Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Engine::Editor::HelpText("(open a node to see its detail)");

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
				Engine::Editor::HelpText("No animation");
			}

			for (size_t _i = 0; _i < _animHandleVec.size(); ++_i)
			{
				ImGui::PushID(static_cast<int>(_i));

				const auto* _pAnim = a_editContext.pServices->pResourceManager->Ref(_animHandleVec[_i]);

				// 未ロードでもアセットとしては飛べるので、リンクだけは出す
				if (!_pAnim)
				{
					const Engine::GUID _animGUID = (_i < _assetData.animationGUIDs.size())
						? _assetData.animationGUIDs[_i]
						: Engine::DefaultGUID;

					const std::string _index = "[" + std::to_string(_i) + "]";
					DrawAssetLink(&a_editContext, _index.c_str(), _animGUID);
					Engine::Editor::SameLine();
					Engine::Editor::HelpText("(not loaded)");

					ImGui::PopID();
					continue;
				}

				if (ImGui::TreeNode("AnimNode", "[%zu] %s", _i, _pAnim->name.c_str()))
				{
					if (_i < _assetData.animationGUIDs.size())
					{
						DrawAssetLink(&a_editContext, "Asset     :", _assetData.animationGUIDs[_i]);
					}
					Engine::Editor::Value("MaxLength", "%.3f frame", _pAnim->maxLength);
					Engine::Editor::Value("AnimNodes", "%zu", _pAnim->nodes.size());

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

							Engine::Editor::Text("%s (node %d) : T=%zu R=%zu S=%zu", _targetName.c_str(), _animNode.nodeOffset, _animNode.translations.size(), _animNode.rotations.size(), _animNode.scales.size());
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
				const Engine::GUID _meshGUID = (_i < _assetData.meshGUIDs.size())
					? _assetData.meshGUIDs[_i]
					: Engine::DefaultGUID;

				ImGui::PushID(static_cast<int>(_i));

				// 中身を見に行けるように、まず飛べる見出しを出す
				const std::string _index = "[" + std::to_string(_i) + "]";
				DrawAssetLink(&a_editContext, _index.c_str(), _meshGUID);

				const auto* _pMesh = a_editContext.pServices->pResourceManager->Ref(_meshHandleVec[_i]);
				if (!_pMesh)
				{
					Engine::Editor::SameLine();
					Engine::Editor::HelpText("(not loaded)");
					ImGui::PopID();
					continue;
				}

				// ここで出すのは概要だけ。詳しくはリンク先のメッシュインスペクタで見る
				const auto& _metaData = _pMesh->GetMetaData();
				Engine::Editor::SameLine();
				Engine::Editor::HelpText("verts=%zu subsets=%zu%s", _pMesh->GetVertexVec().size(), _metaData.subsets.size(), _metaData.isSkinMesh ? " [Skin]" : "");

				ImGui::PopID();
			}
		}

		// ---- マテリアル ----
		if (ImGui::CollapsingHeader("Materials"))
		{
			const auto& _materialHandleVec = a_pModel->GetMaterialHandles();
			for (size_t _i = 0; _i < _materialHandleVec.size(); ++_i)
			{
				const Engine::GUID _materialGUID = (_i < _assetData.materialGUIDs.size())
					? _assetData.materialGUIDs[_i]
					: Engine::DefaultGUID;

				ImGui::PushID(static_cast<int>(_i));

				const auto* _pMaterial = a_editContext.pServices->pResourceManager->Ref(_materialHandleVec[_i]);

				// 表示はマテリアル名を優先する(ファイル名と食い違うことがある)。
				// ロードできていなければアセット名に任せる
				const std::string _index = "[" + std::to_string(_i) + "]";
				DrawAssetLink(&a_editContext, _index.c_str(), _materialGUID,
					_pMaterial ? _pMaterial->name.c_str() : nullptr);

				if (!_pMaterial)
				{
					Engine::Editor::SameLine();
					Engine::Editor::HelpText("(not loaded)");
				}

				ImGui::PopID();
			}
		}

		// ---- 描画コマンド ----
		if (ImGui::CollapsingHeader("Draw Commands"))
		{
			const auto& _drawCommandVec = a_pModel->GetDrawCommandVec();
			Engine::Editor::Value("Count", "%zu", _drawCommandVec.size());

			for (size_t _i = 0; _i < _drawCommandVec.size(); ++_i)
			{
				const auto& _cmd = _drawCommandVec[_i];

				// ノード名が引けるなら名前で表示
				std::string _nodeName = "unknown";
				if (_cmd.nodeIndex < _nodeVec.size())
				{
					_nodeName = _nodeVec[_cmd.nodeIndex].name;
				}

				Engine::Editor::Text("[%zu] node=%s sub=%u mesh=%u(gen%u) material=%u(gen%u) alpha=%s", _i, _nodeName.c_str(), static_cast<UINT>(_cmd.subIdx), static_cast<UINT>(_cmd.meshHandle.GetIndex()), static_cast<UINT>(_cmd.meshHandle.GetGeneration()), static_cast<UINT>(_cmd.materialHandle.GetIndex()), static_cast<UINT>(_cmd.materialHandle.GetGeneration()), std::string(magic_enum::enum_name(_cmd.alphaMode)).c_str());
			}
		}
	}
}
