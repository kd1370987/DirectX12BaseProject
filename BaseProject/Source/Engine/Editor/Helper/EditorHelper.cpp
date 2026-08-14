#include "EditorHelper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Data/Model/Model.h"
#include "../../Resource/Data/Texture/Texture.h"
#include "../../Resource/Data/Animation/Animation.h"

namespace Engine::Editor
{
	//======================================================================================
	// 一覧を絞り込むための検索欄
	//======================================================================================
	const std::string& EditorHelper::DrawSearchBox(const char* a_lable, const char* a_hint, bool a_isAutoFocus)
	{
		// 入力は呼び出し位置(ImGuiのID)ごとに覚える。
		// 呼ぶ側に文字列を持たせずに済ませるためで、
		// アセット選択のように同じ関数を何十箇所からも呼ぶ欄でも入力が混ざらない
		// (IDは窓とIDスタックを含むので、ラベルが同じでも別の欄なら別キーになる)。
		static std::unordered_map<ImGuiID, std::string> s_searchMap;

		std::string& _search = s_searchMap[ImGui::GetID(a_lable)];

		// 開いた直後はそのまま打ち始められるようにする。
		// 前回の入力が残っていると「何も出ない」ように見えるのでここで消す。
		// 出しっぱなしのパネルでは邪魔なので、呼ぶ側が切れるようにしてある。
		if (a_isAutoFocus && ImGui::IsWindowAppearing())
		{
			_search.clear();
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint(a_lable, a_hint, &_search);

		return _search;
	}

	//======================================================================================
	// 検索文字列に引っかかるか
	//======================================================================================
	bool EditorHelper::IsMatchSearch(const std::string& a_search, const std::string& a_text)
	{
		// 未入力なら絞り込まない
		if (a_search.empty()) return true;

		// 大文字小文字を区別しない部分一致。
		// 小文字化したコピーを作らずに済ませたいので std::search で比較器を差し替える
		auto _isSameChar = [](char a_lhs, char a_rhs)
		{
			return
				std::tolower(static_cast<unsigned char>(a_lhs)) ==
				std::tolower(static_cast<unsigned char>(a_rhs));
		};

		return std::search(
			a_text.begin(), a_text.end(),
			a_search.begin(), a_search.end(),
			_isSameChar) != a_text.end();
	}

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
			// 数が増えると探せなくなるので名前で絞り込めるようにする
			const std::string& _search = DrawSearchBox();

			const auto& _assetList = Resource::AssetDatabase::Instance().GetTypeMetaVec(a_assetTypeName);
			for (const auto& _prop : _assetList)
			{
				if (!IsMatchSearch(_search, _prop.fileName)) continue;

				bool _isSelected = (a_currentGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _isSelected))
				{
					a_outSelectedGUID = _prop.guid;
					_isChanged = true;
				}

				// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
				// (絞り込み中は検索欄に入力しているので、そちらからフォーカスを奪わない)
				if (_isSelected && _search.empty())
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
			// ボーンは数が多いので名前で絞り込めるようにする
			const std::string& _search = DrawSearchBox();

			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				if (!IsMatchSearch(_search, _nodes[_i].name)) continue;

				bool _isSelected = (a_inoutNodeIndex == _i);

				if (ImGui::Selectable(_nodes[_i].name.c_str(), _isSelected))
				{
					a_inoutNodeNameHash = _nodes[_i].nodeNameHash;
					a_inoutNodeIndex = static_cast<UINT>(_i);
					_isChanged = true;
				}

				// 絞り込み中は検索欄からフォーカスを奪わない
				if (_isSelected && _search.empty())
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
			// ボーンは数が多いので名前で絞り込めるようにする
			const std::string& _search = DrawSearchBox();

			for (const auto& _node : _nodes)
			{
				if (!IsMatchSearch(_search, _node.name)) continue;

				bool _isSelected = (a_inoutNodeName == _node.name);

				if (ImGui::Selectable(_node.name.c_str(), _isSelected))
				{
					a_inoutNodeName = _node.name;
					a_inoutNodeNameHash = _node.nodeNameHash;
					_isChanged = true;
				}

				// 絞り込み中は検索欄からフォーカスを奪わない
				if (_isSelected && _search.empty())
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
				// 名前で絞り込めるようにする
				const std::string& _search = EditorHelper::DrawSearchBox();

				for (const auto& _ref : a_pModel->GetAnimationHandles())
				{
					const auto* _pAnim = Resource::ResourceManager::Instance().Get(_ref);
					if (!_pAnim) continue;

					if (!EditorHelper::IsMatchSearch(_search, _pAnim->name)) continue;

					bool _isSelected = (_ref == a_currentHandle);

					if (ImGui::Selectable(_pAnim->name.c_str(), _isSelected))
					{
						a_outSelected = _ref;
						_isChanged = true;
					}

					// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
					// (絞り込み中は検索欄からフォーカスを奪わない)
					if (_isSelected && _search.empty())
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
		auto& _resMgr = Resource::ResourceManager::Instance();

		// 読み込み中はまだ中身が空なので、SRVを引くと不正なディスクリプタを掴む
		if (!_resMgr.IsReady(a_handle))
		{
			ImGui::Text("Loading...");
			return { 0,0 };
		}

		auto* _pTex = _resMgr.Ref(a_handle);
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

	//======================================================================================
	// SRVを画像としてImGui上に描画させる
	//======================================================================================
	ImVec2 EditorHelper::DrawSRVView(
		D3D12_GPU_DESCRIPTOR_HANDLE a_gpuHandle,
		float a_width, float a_height,
		float a_minSize, float a_maxSize
	)
	{
		ImTextureID _imTex = (ImTextureID)(a_gpuHandle.ptr);

		// 横幅だけを取得（縦の残り領域は無視する）
		float drawWidth = ImGui::GetContentRegionAvail().x;

		// アスペクト比を計算
		float aspect = a_width / a_height;

		// 横幅に合わせて高さを逆算する
		float drawHeight = drawWidth / aspect;

		ImGui::Text("Size : %f,%f", drawWidth, drawHeight);

		// 計算したサイズで描画
		ImGui::Image(_imTex, ImVec2(drawWidth, drawHeight));

		// 実際に描画したサイズを返す
		return ImVec2(drawWidth, drawHeight);
	}

	namespace
	{
		//----------------------------------------------------------------------------------
		// 行列の成分をそのまま並べる
		//----------------------------------------------------------------------------------
		bool DrawMatrixRaw(DirectX::XMFLOAT4X4& a_mat)
		{
			float* _m = reinterpret_cast<float*>(a_mat.m);

			bool _isEdit = false;

			for (int _row = 0; _row < 4; ++_row)
			{
				ImGui::Text("M%d", _row);
				ImGui::SameLine();

				ImGui::PushID(_row);
				_isEdit |= ImGui::DragFloat4(
					"##Value",
					&_m[_row * 4],
					0.01f,
					-FLT_MAX,
					FLT_MAX,
					"%.3f");
				ImGui::PopID();
			}

			return _isEdit;
		}

		//----------------------------------------------------------------------------------
		// 行列を位置・回転・スケールに分解して表示する
		// 編集されたら分解した値から組み直して書き戻す
		//----------------------------------------------------------------------------------
		bool DrawMatrixPosRotScale(DirectX::XMFLOAT4X4& a_mat)
		{
			DXSM::Matrix _mat = a_mat;

			// 行列の分解
			DXSM::Vector3 _scale = {};
			DXSM::Quaternion _rotQuat = {};
			DXSM::Vector3 _pos = {};
			if (!_mat.Decompose(_scale, _rotQuat, _pos))
			{
				ImGui::Text("Matrix Decompose Failed");
				return false;
			}

			// クォータニオンをEulerに直してDegreeへ変換
			DXSM::Vector3 _rotRad = _rotQuat.ToEuler();
			DXSM::Vector3 _rotDeg = {
				DirectX::XMConvertToDegrees(_rotRad.x),
				DirectX::XMConvertToDegrees(_rotRad.y),
				DirectX::XMConvertToDegrees(_rotRad.z)
			};

			bool _isEdit = false;

			ImGui::Text("Position");
			_isEdit |= ImGui::DragFloat3("##Position", &_pos.x, 0.1f);

			ImGui::Separator();

			ImGui::Text("Rotation");
			_isEdit |= ImGui::DragFloat3("##Rotation", &_rotDeg.x, 0.5f);

			ImGui::Separator();

			ImGui::Text("Scale");
			_isEdit |= ImGui::DragFloat3("##Scale", &_scale.x, 0.1f);

			// 触られたときだけ組み直す(毎フレーム分解->合成すると誤差が乗るため)
			if (_isEdit)
			{
				DXSM::Quaternion _newQuat =
					DXSM::Quaternion::CreateFromYawPitchRoll(
						DirectX::XMConvertToRadians(_rotDeg.y),	// Yaw
						DirectX::XMConvertToRadians(_rotDeg.x),	// Pitch
						DirectX::XMConvertToRadians(_rotDeg.z)	// Roll
					);

				a_mat = DXSM::Matrix::CreateScale(_scale)
					* DXSM::Matrix::CreateFromQuaternion(_newQuat)
					* DXSM::Matrix::CreateTranslation(_pos);
			}

			return _isEdit;
		}
	}

	//======================================================================================
	// 行列の編集
	//======================================================================================
	bool EditorHelper::DrawMatrix(
		const char* a_lable,
		DirectX::XMFLOAT4X4& a_mat,
		EMatrixViewMode a_defaultMode
	)
	{
		bool _isEdit = false;

		ImGui::PushID(a_lable);

		if (ImGui::TreeNodeEx(a_lable, ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 表示形式はImGuiの状態ストレージにラベル単位で覚えさせる
			ImGuiStorage* _pStorage = ImGui::GetStateStorage();
			const ImGuiID _key = ImGui::GetID("MatrixViewMode");

			auto _mode = static_cast<EMatrixViewMode>(
				_pStorage->GetInt(_key, static_cast<int>(a_defaultMode)));

			// 表示形式の切り替え
			if (DrawEnumCombo("ViewMode", _mode))
			{
				_pStorage->SetInt(_key, static_cast<int>(_mode));
			}
			ImGui::Separator();

			switch (_mode)
			{
			case EMatrixViewMode::PosRotScale:
				_isEdit = DrawMatrixPosRotScale(a_mat);
				break;

			case EMatrixViewMode::Raw:
			default:
				_isEdit = DrawMatrixRaw(a_mat);
				break;
			}

			ImGui::TreePop();
		}

		ImGui::PopID();

		return _isEdit;
	}

	//======================================================================================
	// クォータニオンを度数法のオイラー角として編集する
	//======================================================================================
	bool EditorHelper::DragRotationDeg3FromQuaternion(DirectX::XMFLOAT4& a_quat)
	{
		DXSM::Quaternion _quat = a_quat;
		DXSM::Vector3 _rotRad = _quat.ToEuler();

		// Degreeへ変換
		DXSM::Vector3 _rotDeg = {
			DirectX::XMConvertToDegrees(_rotRad.x),
			DirectX::XMConvertToDegrees(_rotRad.y),
			DirectX::XMConvertToDegrees(_rotRad.z)
		};

		ImGui::Text("Rotation");
		// 値が変更されたか取得
		if (ImGui::DragFloat3("##Rotation", &_rotDeg.x, 0.5f))
		{
			// Degree → Radian
			_rotRad = {
				DirectX::XMConvertToRadians(_rotDeg.x),
				DirectX::XMConvertToRadians(_rotDeg.y),
				DirectX::XMConvertToRadians(_rotDeg.z)
			};

			// Euler → Quaternion
			DXSM::Quaternion _newQuat =
				DXSM::Quaternion::CreateFromYawPitchRoll(
					_rotRad.y, // Yaw
					_rotRad.x, // Pitch
					_rotRad.z  // Roll
				);

			// 引数へ反映
			a_quat = {
				_newQuat.x,
				_newQuat.y,
				_newQuat.z,
				_newQuat.w
			};
			return true;
		}

		return false;
	}

	//======================================================================================
	// ノードのタイトルバー表示
	//======================================================================================
	void EditorHelper::DrawNodeTitleBar(const std::string& a_name)
	{
		ImNodes::BeginNodeTitleBar();
		ImGui::Text("%s", a_name.c_str());
		ImNodes::EndNodeTitleBar();
	}
}
