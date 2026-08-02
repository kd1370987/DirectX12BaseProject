#include "EditorHelper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Data/Model/Model.h"
#include "../../Resource/Data/Texture/Texture.h"
#include "../../Resource/Data/Animation/Animation.h"

namespace Engine::Editor
{
	//======================================================================================
	// アセットデータベースから1件選ばせるだけの土台
	//======================================================================================
	bool EditorHelper::DrawAssetGUIDCombo(
		const char* a_lable,
		const char* a_assetTypeName,
		const GUID& a_currentGUID,
		GUID& a_outSelectedGUID
	)
	{
		bool _isChanged = false;

		// 現在の選択情報 : ハンドルを持たない前提なのでGUIDから名前を引く
		auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(a_currentGUID);
		if (!_fileName.empty())
		{
			ImGui::Text("%s : %s", a_assetTypeName, _fileName.c_str());
			ImGui::Text("%s", a_currentGUID.String().c_str());
		}

		// 選択UI
		if (ImGui::BeginCombo(a_lable, "Select..."))
		{
			const auto& _assetList = Resource::AssetDatabase::Instance().GetTypeMetaVec(a_assetTypeName);
			for (const auto& _prop : _assetList)
			{
				bool _isSelected = (a_currentGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _isSelected))
				{
					a_outSelectedGUID = _prop.guid;
					_isChanged = true;
				}

				// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	//======================================================================================
	// GUIDのみを書き換えるアセット検索欄
	//======================================================================================
	bool EditorHelper::DrawAssetSelectComboGUID(
		const char* a_lable,
		const char* a_assetTypeName,
		GUID& a_inoutGUID
	)
	{
		GUID _selectedGUID = {};
		if (!DrawAssetGUIDCombo(a_lable, a_assetTypeName, a_inoutGUID, _selectedGUID))
		{
			return false;
		}

		a_inoutGUID = _selectedGUID;
		return true;
	}

	//======================================================================================
	// モデルのノードをインデックスで選択する
	//======================================================================================
	bool EditorHelper::DrawModelNodeCombo(
		const char* a_lable,
		const Resource::Model* a_pModel,
		UINT& a_inoutNodeIndex,
		UINT& a_inoutNodeNameHash
	)
	{
		if (!a_pModel)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
			return false;
		}

		// モデルが管理する全ノード配列
		const auto& _nodes = a_pModel->GetOriginalNodeVec();

		// 現在選択されているノード名を表示用として取得
		std::string _currentNodeName = "None / Invalid";
		if (a_inoutNodeIndex < _nodes.size())
		{
			_currentNodeName = _nodes[a_inoutNodeIndex].name;
		}

		bool _isChanged = false;

		if (ImGui::BeginCombo(a_lable, _currentNodeName.c_str()))
		{
			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				bool _isSelected = (a_inoutNodeIndex == _i);

				if (ImGui::Selectable(_nodes[_i].name.c_str(), _isSelected))
				{
					a_inoutNodeNameHash = _nodes[_i].nodeNameHash;
					a_inoutNodeIndex = static_cast<UINT>(_i);
					_isChanged = true;
				}

				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	//======================================================================================
	// モデルのノードを名前で選択する
	//======================================================================================
	bool EditorHelper::DrawModelNodeComboByName(
		const char* a_lable,
		const Resource::Model* a_pModel,
		std::string& a_inoutNodeName,
		UINT& a_inoutNodeNameHash
	)
	{
		if (!a_pModel)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
			return false;
		}

		const auto& _nodes = a_pModel->GetOriginalNodeVec();

		// 現在の選択表示
		std::string _currentNodeName = a_inoutNodeName.empty() ? "Select node..." : a_inoutNodeName;

		bool _isChanged = false;

		if (ImGui::BeginCombo(a_lable, _currentNodeName.c_str()))
		{
			for (const auto& _node : _nodes)
			{
				bool _isSelected = (a_inoutNodeName == _node.name);

				if (ImGui::Selectable(_node.name.c_str(), _isSelected))
				{
					a_inoutNodeName = _node.name;
					a_inoutNodeNameHash = _node.nodeNameHash;
					_isChanged = true;
				}

				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	//======================================================================================
	// モデルが持つアニメーションを選択する
	//======================================================================================
	namespace
	{
		// アニメーション選択コンボの本体
		// ハンドルの持ち方(生ハンドル / 参照カウント付き)だけが違うので、選択結果だけを返す
		bool DrawModelAnimationComboImpl(
			const char* a_lable,
			const Resource::Model* a_pModel,
			const Handle<Resource::AnimationData>& a_currentHandle,
			Handle<Resource::AnimationData>& a_outSelected
		)
		{
			if (!a_pModel) { return false; }

			// 現在の再生アニメ名をプレビューにする
			std::string _viewName = "Select...";
			const auto* _pCurrentAnim = Resource::ResourceManager::Instance().Get(a_currentHandle);
			if (_pCurrentAnim)
			{
				_viewName = _pCurrentAnim->name;
			}

			bool _isChanged = false;

			if (ImGui::BeginCombo(a_lable, _viewName.c_str()))
			{
				for (const auto& _ref : a_pModel->GetAnimationHandles())
				{
					const auto* _pAnim = Resource::ResourceManager::Instance().Get(_ref);
					if (!_pAnim) continue;

					bool _isSelected = (_ref == a_currentHandle);

					if (ImGui::Selectable(_pAnim->name.c_str(), _isSelected))
					{
						a_outSelected = _ref;
						_isChanged = true;
					}

					// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
					if (_isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			return _isChanged;
		}
	}

	bool EditorHelper::DrawModelAnimationCombo(
		const char* a_lable,
		const Resource::Model* a_pModel,
		Handle<Resource::AnimationData>& a_inoutHandle
	)
	{
		Handle<Resource::AnimationData> _selected = {};
		if (!DrawModelAnimationComboImpl(a_lable, a_pModel, a_inoutHandle, _selected))
		{
			return false;
		}

		a_inoutHandle = _selected;
		return true;
	}

	bool EditorHelper::DrawModelAnimationCombo(
		const char* a_lable,
		const Resource::Model* a_pModel,
		ResourceRef<Resource::AnimationData>& a_inoutRef
	)
	{
		Handle<Resource::AnimationData> _selected = {};
		if (!DrawModelAnimationComboImpl(a_lable, a_pModel, a_inoutRef.GetRaw(), _selected))
		{
			return false;
		}

		// 参照カウントの付け替えはResourceRef側に任せる
		a_inoutRef = ResourceRef<Resource::AnimationData>(_selected);
		return true;
	}

	//======================================================================================
	// エディター上でテクスチャを表示する
	//======================================================================================
	ImVec2 EditorHelper::DrawTexture(
		const Handle<Resource::Texture>& a_handle,
		float a_width,
		float a_height
	)
	{
		// テクスチャ表示
		auto* _pTex = Resource::ResourceManager::Instance().Ref(a_handle);
		if (!_pTex)
		{
			ImGui::Text("Not find texture");
			return { 0,0 };
		}
		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());

		ImTextureID _imTex = (ImTextureID)(_gpuHandle.ptr);

		// 横幅だけを取得（縦の残り領域は無視する）
		float drawWidth = ImGui::GetContentRegionAvail().x;

		// アスペクト比を計算
		float aspect = a_width / a_height;

		// 横幅に合わせて高さを逆算する
		float drawHeight = drawWidth / aspect;

		// 計算したサイズで描画
		ImGui::Image(_imTex, ImVec2(drawWidth, drawHeight));

		// 実際に描画したサイズを返す
		return ImVec2(drawWidth, drawHeight);
	}
}
