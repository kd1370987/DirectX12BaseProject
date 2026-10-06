#include "AnimationEdit.h"

namespace Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// アニメーションの詳細表示
	//-----------------------------------------------------------------------------------------
	void AnimationEdit(EditorContext&, Resource::AnimationData* a_pAnimation)
	{
		if (!a_pAnimation) { return; }

		// ---- 概要 ----
		Engine::EditorField::Value("Name", "%s", a_pAnimation->name.c_str());
		Engine::EditorField::Value("MaxLength", "%.3f frame", a_pAnimation->maxLength);
		Engine::EditorField::Value("AnimNodes", "%zu", a_pAnimation->nodes.size());

		// 全チャンネルのキー総数
		size_t _totalKeyCount = 0;
		for (const auto& _animNode : a_pAnimation->nodes)
		{
			_totalKeyCount += _animNode.translations.size();
			_totalKeyCount += _animNode.rotations.size();
			_totalKeyCount += _animNode.scales.size();
		}
		Engine::EditorField::Value("TotalKeys", "%zu", _totalKeyCount);

		Engine::EditorField::Line();

		// ---- アニメーションノード ----
		if (!ImGui::CollapsingHeader("Animation Nodes", ImGuiTreeNodeFlags_DefaultOpen)) { return; }

		for (size_t _i = 0; _i < a_pAnimation->nodes.size(); ++_i)
		{
			const auto& _animNode = a_pAnimation->nodes[_i];

			ImGui::PushID(static_cast<int>(_i));
			if (ImGui::TreeNode("AnimNode", "[%zu] node offset : %d", _i, _animNode.nodeOffset))
			{
				Engine::EditorField::Value("Translation Keys", "%zu", _animNode.translations.size());
				Engine::EditorField::Value("Rotation Keys", "%zu", _animNode.rotations.size());
				Engine::EditorField::Value("Scale Keys", "%zu", _animNode.scales.size());

				// 座標キー
				if (!_animNode.translations.empty() && ImGui::TreeNode("Translations"))
				{
					for (const auto& _key : _animNode.translations)
					{
						Engine::EditorField::Text("%8.3f : %.3f, %.3f, %.3f", _key.time, _key.vec.x, _key.vec.y, _key.vec.z);
					}
					ImGui::TreePop();
				}

				// 回転キー
				if (!_animNode.rotations.empty() && ImGui::TreeNode("Rotations"))
				{
					for (const auto& _key : _animNode.rotations)
					{
						Engine::EditorField::Text("%8.3f : %.3f, %.3f, %.3f, %.3f", _key.time, _key.quat.x, _key.quat.y, _key.quat.z, _key.quat.w);
					}
					ImGui::TreePop();
				}

				// 拡縮キー
				if (!_animNode.scales.empty() && ImGui::TreeNode("Scales"))
				{
					for (const auto& _key : _animNode.scales)
					{
						Engine::EditorField::Text("%8.3f : %.3f, %.3f, %.3f", _key.time, _key.vec.x, _key.vec.y, _key.vec.z);
					}
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
}
