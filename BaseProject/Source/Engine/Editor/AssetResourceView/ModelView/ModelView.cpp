#include "ModelView.h"
//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Loader/Model/ModelLoader.h"

#include "../../ImGui/ImGuiHelper/ImGuiHelper.h"

namespace Engine::Editor
{
	void ModelView::DrawModel(const Engine::GUID& a_guid)
	{
		// GUIDからハンドルを取得
		auto _handle = Resource::ModelLoader::GetHandle(a_guid);

		// ハンドルからモデルを取得
		if (_handle == Handle<Resource::Model>()) return;
		auto* _pModel = Resource::ResourceManager::Instance().Ref(_handle);

		// アニメーション表示
		if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
		{
			AnimationView(*_pModel);
			ImGui::TreePop();
		}

		// ノード配列表示
		if (ImGui::TreeNodeEx("Node", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
		{
			DrawModelView(*_pModel);
			ImGui::TreePop();
		}

		ImGui::Separator();
		// セーブボタン
		if (ImGui::Button("Save"))
		{
			auto _path = Resource::ModelLoader::GetFilePath(_handle);
			_pModel->Save(_path);
		}
	}


	void ModelView::DrawModelView(const Engine::Resource::Model& a_model)
	{
		// オリジナルノード
		for(size_t _i = 0; _i < a_model.GetOriginalNodeVec().size(); ++_i)
		{
			auto& _node = a_model.GetOriginalNodeVec()[_i];

			ImGui::PushID(static_cast<int>(_i));

			std::string _tagName = _node.name;
			if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				NodeView(_node);

				// ノード描画
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}

	void ModelView::NodeView(const Engine::Resource::Node& a_node)
	{
		// ローカル座標表示
		DXSM::Matrix _lMat = a_node.localTransform;
		Editor::Helper::DrawMatrixForPOS_ROT_SCALE("LocalMat",_lMat);
		// ワールド座標表示
		DXSM::Matrix _wMat = a_node.worldTransform;
		Editor::Helper::DrawMatrixForPOS_ROT_SCALE("WorldMat",_wMat);

	}

	void ModelView::AnimationView(const Engine::Resource::Model& a_model)
	{
		auto& _aniHandleVec = a_model.GetAnimationHandles();
		for(size_t _i = 0; _i < _aniHandleVec.size(); ++_i)
		{
			auto& _aniHandle = _aniHandleVec[_i];

			// アニメーションデータ取得
			const auto* _pAniData = Resource::ResourceManager::Instance().Get(_aniHandle);
			if (!_pAniData) continue;

			ImGui::PushID(static_cast<int>(_i));

			if (ImGui::TreeNodeEx(_pAniData->name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				ImGui::Text("MaxTime");
				ImGui::Text("%f", _pAniData->maxLength);

				ImGui::TreePop();
			}

			ImGui::PopID();

		}

	}
}