#include "EditorField.h"
#include "EditorHelper.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../Resource/Data/Model/Model.h"
#include "../../Resource/Data/Texture/Texture.h"
#include "../../Resource/Data/Animation/Animation.h"

// ImGuizmo はギズモを触る側だけで使う。
// プリコンパイル済みヘッダーへ置くと全翻訳単位に広がるため
#pragma warning(push, 0)
#include <imGuizmo.h>
#pragma warning(pop)

namespace Engine::Editor
{
	namespace
	{
		ImVec2 ToImVec2(const Math::Vector2& a_value)
		{
			return ImVec2(a_value.x, a_value.y);
		}

		ImVec4 ToImVec4(const Math::Color& a_value)
		{
			return ImVec4(a_value.r, a_value.g, a_value.b, a_value.a);
		}

		// 小数の既定の書式
		const char* FloatFormat(const char* a_format)
		{
			return a_format ? a_format : "%.3f";
		}

		// 警告の文字色
		const Math::Color WARNING_COLOR = Math::Color(1.0f, 1.0f, 0.0f, 1.0f);
	}

	//======================================================================================
	// 文字
	//======================================================================================
	void Text(const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::TextV(a_fmt, _args);
		va_end(_args);
	}

	void HelpText(const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::TextDisabledV(a_fmt, _args);
		va_end(_args);
	}

	void TextColored(const Math::Color& a_color, const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::TextColoredV(ToImVec4(a_color), a_fmt, _args);
		va_end(_args);
	}

	void LabelText(const char* a_label, const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::LabelTextV(a_label, a_fmt, _args);
		va_end(_args);
	}

	void BulletText(const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::BulletTextV(a_fmt, _args);
		va_end(_args);
	}

	void Tooltip(const char* a_fmt, ...)
	{
		va_list _args;
		va_start(_args, a_fmt);
		ImGui::SetItemTooltipV(a_fmt, _args);
		va_end(_args);
	}

	//======================================================================================
	// 区切り・配置
	//======================================================================================
	void Line()
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}

	void Separator()
	{
		ImGui::Separator();
	}

	void Section(const char* a_label)
	{
		ImGui::SeparatorText(a_label);
	}

	void Spacing()
	{
		ImGui::Spacing();
	}

	void SameLine()
	{
		ImGui::SameLine();
	}

	void SetNextItemWidth(float a_width)
	{
		ImGui::SetNextItemWidth(a_width);
	}

	//======================================================================================
	// 値の変更
	//======================================================================================
	bool Field(const char* a_label, bool& a_value)
	{
		return ImGui::Checkbox(a_label, &a_value);
	}

	bool Field(const char* a_label, int& a_value, float a_speed, int a_min, int a_max)
	{
		return ImGui::DragInt(a_label, &a_value, a_speed, a_min, a_max);
	}

	bool Field(const char* a_label, int (&a_values)[2], float a_speed, int a_min, int a_max)
	{
		return ImGui::DragInt2(a_label, a_values, a_speed, a_min, a_max);
	}

	bool Field(const char* a_label, uint32_t& a_value)
	{
		return ImGui::InputScalar(a_label, ImGuiDataType_U32, &a_value);
	}

	bool Field(const char* a_label, uint64_t& a_value)
	{
		return ImGui::InputScalar(a_label, ImGuiDataType_U64, &a_value);
	}

	bool Field(const char* a_label, float& a_value, float a_speed, float a_min, float a_max, const char* a_format)
	{
		return ImGui::DragFloat(a_label, &a_value, a_speed, a_min, a_max, FloatFormat(a_format));
	}

	bool Field(const char* a_label, Math::Vector2& a_value, float a_speed, float a_min, float a_max, const char* a_format)
	{
		return ImGui::DragFloat2(a_label, &a_value.x, a_speed, a_min, a_max, FloatFormat(a_format));
	}

	bool Field(const char* a_label, Math::Vector3& a_value, float a_speed, float a_min, float a_max, const char* a_format)
	{
		return ImGui::DragFloat3(a_label, &a_value.x, a_speed, a_min, a_max, FloatFormat(a_format));
	}

	//--------------------------------------------------------------------------------------
	// 回転 : 度数法のオイラー角として編集する
	//--------------------------------------------------------------------------------------
	bool Field(const char* a_label, Math::Quaternion& a_value)
	{
		const Math::Vector3 _rotRad = a_value.ToEuler();

		// Degreeへ変換
		Math::Vector3 _rotDeg = {
			DirectX::XMConvertToDegrees(_rotRad.x),
			DirectX::XMConvertToDegrees(_rotRad.y),
			DirectX::XMConvertToDegrees(_rotRad.z)
		};

		if (!ImGui::DragFloat3(a_label, &_rotDeg.x, 0.5f)) return false;

		// Euler(Degree) → Quaternion
		const Math::Quaternion _newQuat =
			Math::Quaternion::CreateFromYawPitchRoll(
				DirectX::XMConvertToRadians(_rotDeg.y),	// Yaw
				DirectX::XMConvertToRadians(_rotDeg.x),	// Pitch
				DirectX::XMConvertToRadians(_rotDeg.z)	// Roll
			);

		a_value = { _newQuat.x, _newQuat.y, _newQuat.z, _newQuat.w };
		return true;
	}

	//--------------------------------------------------------------------------------------
	// 色(RGBA) : ピッカーは 0..1 しか触れないので、1 を超える値のために数値のドラッグも出す
	//--------------------------------------------------------------------------------------
	bool Field(const char* a_label, Math::Color& a_value)
	{
		bool _isChange = false;

		constexpr ImGuiColorEditFlags _kFlags =
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_HDR |
			ImGuiColorEditFlags_AlphaBar |
			ImGuiColorEditFlags_AlphaPreviewHalf;

		ImGui::PushID(a_label);
		_isChange |= ImGui::ColorEdit4(a_label, a_value.Data(), _kFlags);
		_isChange |= ImGui::DragFloat4("##ColorValue", a_value.Data(), 0.01f, 0.0f, 0.0f, "%.2f");
		ImGui::PopID();

		return _isChange;
	}

	//--------------------------------------------------------------------------------------
	// 行列
	//--------------------------------------------------------------------------------------
	namespace
	{
		// 行列の成分をそのまま並べる
		bool DrawMatrixRaw(Math::Matrix& a_mat)
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

		// 行列を位置・回転・スケールに分解して表示する
		// 編集されたら分解した値から組み直して書き戻す
		bool DrawMatrixPosRotScale(Math::Matrix& a_mat)
		{
			// 行列の分解
			Math::Vector3 _scale = {};
			Math::Quaternion _rotQuat = {};
			Math::Vector3 _pos = {};
			if (!a_mat.Decompose(_scale, _rotQuat, _pos))
			{
				ImGui::Text("Matrix Decompose Failed");
				return false;
			}

			// クォータニオンをEulerに直してDegreeへ変換
			Math::Vector3 _rotRad = _rotQuat.ToEuler();
			Math::Vector3 _rotDeg = {
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
				Math::Quaternion _newQuat =
					Math::Quaternion::CreateFromYawPitchRoll(
						DirectX::XMConvertToRadians(_rotDeg.y),	// Yaw
						DirectX::XMConvertToRadians(_rotDeg.x),	// Pitch
						DirectX::XMConvertToRadians(_rotDeg.z)	// Roll
					);

				a_mat = Math::Matrix::CreateTRS(_pos, _newQuat, _scale);
			}

			return _isEdit;
		}
	}

	bool Field(const char* a_label, Math::Matrix& a_value, EMatrixViewMode a_defaultMode)
	{
		bool _isEdit = false;

		ImGui::PushID(a_label);

		if (ImGui::TreeNodeEx(a_label, ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 表示形式はImGuiの状態ストレージにラベル単位で覚えさせる
			ImGuiStorage* _pStorage = ImGui::GetStateStorage();
			const ImGuiID _key = ImGui::GetID("MatrixViewMode");

			auto _mode = static_cast<EMatrixViewMode>(
				_pStorage->GetInt(_key, static_cast<int>(a_defaultMode)));

			// 表示形式の切り替え
			if (Field("ViewMode", _mode))
			{
				_pStorage->SetInt(_key, static_cast<int>(_mode));
			}
			ImGui::Separator();

			switch (_mode)
			{
			case EMatrixViewMode::PosRotScale:
				_isEdit = DrawMatrixPosRotScale(a_value);
				break;

			case EMatrixViewMode::Raw:
			default:
				_isEdit = DrawMatrixRaw(a_value);
				break;
			}

			ImGui::TreePop();
		}

		ImGui::PopID();

		return _isEdit;
	}

	//--------------------------------------------------------------------------------------
	// 文字列
	//--------------------------------------------------------------------------------------
	bool Field(const char* a_label, std::string& a_value)
	{
		return ImGui::InputText(a_label, &a_value);
	}

	bool Field(const char* a_label, char* a_buff, size_t a_buffSize)
	{
		return ImGui::InputText(a_label, a_buff, a_buffSize);
	}

	bool MultilineField(const char* a_label, std::string& a_value)
	{
		return ImGui::InputTextMultiline(a_label, &a_value);
	}

	//--------------------------------------------------------------------------------------
	// そのほかの数値
	//--------------------------------------------------------------------------------------
	bool Slider(const char* a_label, float& a_value, float a_min, float a_max)
	{
		return ImGui::SliderFloat(a_label, &a_value, a_min, a_max);
	}

	bool RangeField(const char* a_label, float& a_min, float& a_max, float a_speed, float a_lowerLimit, float a_upperLimit)
	{
		return ImGui::DragFloatRange2(a_label, &a_min, &a_max, a_speed, a_lowerLimit, a_upperLimit);
	}

	//--------------------------------------------------------------------------------------
	// 色見本
	//--------------------------------------------------------------------------------------
	bool ColorField(const char* a_label, Math::Color& a_value, bool a_isAlpha)
	{
		// アルファなしのときは RGB の3つだけを書き戻す(アルファは触らない)
		return a_isAlpha
			? ImGui::ColorEdit4(a_label, a_value.Data())
			: ImGui::ColorEdit3(a_label, a_value.Data());
	}

	bool ColorField(const char* a_label, Math::Vector3& a_rgb)
	{
		return ImGui::ColorEdit3(a_label, &a_rgb.x);
	}

	bool ColorPicker(const char* a_label, Math::Color& a_value)
	{
		return ImGui::ColorPicker4(a_label, a_value.Data());
	}

	bool Combo(const char* a_label, int& a_index, const char* const a_items[], int a_count)
	{
		return ImGui::Combo(a_label, &a_index, a_items, a_count);
	}

	bool IsItemEditFinished()
	{
		return ImGui::IsItemDeactivatedAfterEdit();
	}

	//--------------------------------------------------------------------------------------
	// Enum
	//--------------------------------------------------------------------------------------
	bool Internal::EnumCombo(const char* a_label, size_t& a_index, std::span<const std::string_view> a_names)
	{
		// magic_enum の名前は終端付きの静的文字列なので data() をそのまま渡せる
		const char* _preview = (a_index < a_names.size()) ? a_names[a_index].data() : "";

		bool _isChange = false;

		if (ImGui::BeginCombo(a_label, _preview))
		{
			for (size_t _i = 0; _i < a_names.size(); ++_i)
			{
				const bool _isSelect = (_i == a_index);

				if (ImGui::Selectable(a_names[_i].data(), _isSelect))
				{
					a_index = _i;
					_isChange = true;
				}

				if (_isSelect)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		return _isChange;
	}

	bool Internal::FlagsCombo(const char* a_label, uint64_t& a_raw,
		std::span<const uint64_t> a_bits, std::span<const std::string_view> a_names)
	{
		// 立っているビットの名前を並べてプレビューにする
		std::string _preview = "None";
		if (a_raw != 0)
		{
			_preview.clear();
			for (size_t _i = 0; _i < a_bits.size(); ++_i)
			{
				if (a_bits[_i] == 0 || (a_raw & a_bits[_i]) == 0) continue;

				if (!_preview.empty()) _preview += " | ";
				_preview += a_names[_i];
			}
		}

		bool _isChange = false;

		if (ImGui::BeginCombo(a_label, _preview.c_str()))
		{
			for (size_t _i = 0; _i < a_bits.size(); ++_i)
			{
				// 0 の項目(None)は選ぶ意味がないので出さない
				if (a_bits[_i] == 0) continue;

				const bool _isCheck = (a_raw & a_bits[_i]) != 0;

				if (ImGui::Selectable(a_names[_i].data(), _isCheck))
				{
					a_raw = _isCheck ? (a_raw & ~a_bits[_i]) : (a_raw | a_bits[_i]);
					_isChange = true;
				}
			}
			ImGui::EndCombo();
		}

		return _isChange;
	}

	//======================================================================================
	// ボタン・選択
	//======================================================================================
	bool Button(const char* a_label, const Math::Vector2& a_size)
	{
		return ImGui::Button(a_label, ToImVec2(a_size));
	}

	bool SmallButton(const char* a_label)
	{
		return ImGui::SmallButton(a_label);
	}

	bool ArrowButton(const char* a_id, EArrowDir a_dir)
	{
		ImGuiDir _dir = ImGuiDir_Up;
		switch (a_dir)
		{
		case EArrowDir::Left:	_dir = ImGuiDir_Left;	break;
		case EArrowDir::Right:	_dir = ImGuiDir_Right;	break;
		case EArrowDir::Up:		_dir = ImGuiDir_Up;		break;
		case EArrowDir::Down:	_dir = ImGuiDir_Down;	break;
		}
		return ImGui::ArrowButton(a_id, _dir);
	}

	//--------------------------------------------------------------------------------------
	// 生成/削除ボタン
	//
	// 色は「通常 / ホバー / 押下」の3つを差し替える。3つとも入れないと、
	// カーソルを乗せた瞬間に既定色へ戻って別のボタンに見えてしまう。
	// 文字色は触らない(テーマ側の可読性設定をそのまま活かす)。
	//--------------------------------------------------------------------------------------
	namespace
	{
		// 生成系(緑)
		constexpr ImVec4 CREATE_COLOR         = { 0.20f, 0.52f, 0.24f, 1.0f };
		constexpr ImVec4 CREATE_HOVERED_COLOR = { 0.26f, 0.66f, 0.30f, 1.0f };
		constexpr ImVec4 CREATE_ACTIVE_COLOR  = { 0.16f, 0.44f, 0.20f, 1.0f };

		// 削除系(赤)
		constexpr ImVec4 DELETE_COLOR         = { 0.58f, 0.20f, 0.20f, 1.0f };
		constexpr ImVec4 DELETE_HOVERED_COLOR = { 0.72f, 0.26f, 0.26f, 1.0f };
		constexpr ImVec4 DELETE_ACTIVE_COLOR  = { 0.48f, 0.16f, 0.16f, 1.0f };

		void PushButtonColor(const ImVec4& a_normal, const ImVec4& a_hovered, const ImVec4& a_active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, a_normal);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, a_hovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, a_active);
		}

		void PopButtonColor()
		{
			ImGui::PopStyleColor(3);
		}
	}

	bool CreateButton(const char* a_label, const Math::Vector2& a_size)
	{
		PushButtonColor(CREATE_COLOR, CREATE_HOVERED_COLOR, CREATE_ACTIVE_COLOR);
		const bool _isPressed = ImGui::Button(a_label, ToImVec2(a_size));
		PopButtonColor();
		return _isPressed;
	}

	bool CreateSmallButton(const char* a_label)
	{
		PushButtonColor(CREATE_COLOR, CREATE_HOVERED_COLOR, CREATE_ACTIVE_COLOR);
		const bool _isPressed = ImGui::SmallButton(a_label);
		PopButtonColor();
		return _isPressed;
	}

	bool DeleteButton(const char* a_label, const Math::Vector2& a_size)
	{
		PushButtonColor(DELETE_COLOR, DELETE_HOVERED_COLOR, DELETE_ACTIVE_COLOR);
		const bool _isPressed = ImGui::Button(a_label, ToImVec2(a_size));
		PopButtonColor();
		return _isPressed;
	}

	bool DeleteSmallButton(const char* a_label)
	{
		PushButtonColor(DELETE_COLOR, DELETE_HOVERED_COLOR, DELETE_ACTIVE_COLOR);
		const bool _isPressed = ImGui::SmallButton(a_label);
		PopButtonColor();
		return _isPressed;
	}

	bool RadioButton(const char* a_label, bool a_isActive)
	{
		return ImGui::RadioButton(a_label, a_isActive);
	}

	bool Selectable(const char* a_label, bool a_isSelected)
	{
		return ImGui::Selectable(a_label, a_isSelected);
	}

	void SetItemDefaultFocus()
	{
		ImGui::SetItemDefaultFocus();
	}

	bool CollapsingHeader(const char* a_label, bool a_isDefaultOpen)
	{
		return ImGui::CollapsingHeader(a_label, a_isDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
	}

	//======================================================================================
	// 表示
	//======================================================================================
	void ProgressBar(float a_fraction, const char* a_overlay)
	{
		ImGui::ProgressBar(a_fraction, ImVec2(-FLT_MIN, 0.0f), a_overlay);
	}

	Math::Vector2 Image(
		const ECS::EngineServices& a_services,
		const Handle<Resource::Texture>& a_handle,
		float a_width,
		float a_height
	)
	{
		auto& _resMgr = *a_services.pResourceManager;

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
		auto _gpuHandle = EditorHelper::GetImGuiTexHandle(_pTex->GetImGuiSRV());

		ImTextureID _imTex = (ImTextureID)(_gpuHandle.ptr);

		// 横幅だけを取得（縦の残り領域は無視する）
		const float _drawWidth = ImGui::GetContentRegionAvail().x;

		// 横幅に合わせて、指定の比率で高さを逆算する
		const float _aspect = a_width / a_height;
		const float _drawHeight = _drawWidth / _aspect;

		ImGui::Image(_imTex, ImVec2(_drawWidth, _drawHeight));

		// 実際に描画したサイズを返す
		return Math::Vector2(_drawWidth, _drawHeight);
	}

	//======================================================================================
	// アセット選択
	//======================================================================================
	bool AssetPicker(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		const GUID& a_currentGUID,
		GUID& a_outSelectedGUID
	)
	{
		bool _isChanged = false;

		if (!a_services.pAssetDatabase) return false;
		auto& _assetDB = *a_services.pAssetDatabase;

		// 現在の選択情報 : ハンドルを持たない前提なのでGUIDから名前を引く。
		// 同名のアセットが別フォルダにあり得るので、置き場所も一緒に出す
		auto _fileName = _assetDB.GetFileNameFromGUID(a_currentGUID);
		if (!_fileName.empty())
		{
			ImGui::Text("%s : %s", a_assetTypeName, _fileName.c_str());

			if (const auto* _pProp = _assetDB.GetAssetProperty(a_currentGUID))
			{
				ImGui::TextDisabled("%s", _pProp->filePath.c_str());
			}
			ImGui::Text("%s", a_currentGUID.String().c_str());
		}

		// 選択UI
		if (ImGui::BeginCombo(a_label, "Select..."))
		{
			// 数が増えると探せなくなるので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			const auto& _assetList = _assetDB.GetTypeMetaVec(a_assetTypeName);

			// 同名のアセットは名前だけでは選び分けられないので、置き場所を添える対象を先に拾う
			const auto _duplicatedSet = CollectDuplicatedNames(
				_assetList, [](const Resource::AssetProperty& a_prop) { return a_prop.fileName; });

			for (size_t _i = 0; _i < _assetList.size(); ++_i)
			{
				const auto& _prop = _assetList[_i];

				// 同名が並ぶときはフォルダ名で絞り込めたほうが早いので、パスも検索対象にする
				if (!EditorHelper::IsMatchSearch(_search, _prop.fileName) &&
					!EditorHelper::IsMatchSearch(_search, _prop.filePath)) continue;

				bool _isSelected = (a_currentGUID == _prop.guid);

				// 同名でもImGuiのIDがぶつからないようにする。
				// Selectable のIDはラベル文字列から作られるので、名前が同じだと
				// 別のアセットが同じ項目として扱われてしまう
				ImGui::PushID(static_cast<int>(_i));

				const std::string _label = MakeUniqueLabel(
					_duplicatedSet, _prop.fileName, Engine::File::GetDirFromPath(_prop.filePath));

				// 選択欄
				if (ImGui::Selectable(_label.c_str(), _isSelected))
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

				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	bool AssetField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		GUID& a_inoutGUID
	)
	{
		GUID _selectedGUID = {};
		if (!AssetPicker(a_services, a_label, a_assetTypeName, a_inoutGUID, _selectedGUID))
		{
			return false;
		}

		a_inoutGUID = _selectedGUID;
		return true;
	}

	//--------------------------------------------------------------------------------------
	// モデルのノード
	//--------------------------------------------------------------------------------------
	bool ModelNodeField(
		const char* a_label,
		const Resource::Model* a_pModel,
		UINT& a_inoutNodeIndex,
		UINT& a_inoutNodeNameHash
	)
	{
		if (!a_pModel)
		{
			TextColored(WARNING_COLOR, "Warning: Model Resource is null.");
			return false;
		}

		// モデルが管理する全ノード配列
		const auto& _nodes = a_pModel->GetOriginalNodeVec();

		// 現在選択されているノード名を表示用として取得。
		// 同名のノードがあるモデルもあるので、番号まで出して区別できるようにする
		std::string _currentNodeName = "None / Invalid";
		if (a_inoutNodeIndex < _nodes.size())
		{
			_currentNodeName = _nodes[a_inoutNodeIndex].name +
				"   [#" + std::to_string(a_inoutNodeIndex) + "]";
		}

		bool _isChanged = false;

		if (ImGui::BeginCombo(a_label, _currentNodeName.c_str()))
		{
			// ボーンは数が多いので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			// 同名のノードは名前だけでは選び分けられないので、番号を添える対象を先に拾う
			const auto _duplicatedSet = CollectDuplicatedNames(
				_nodes, [](const auto& a_node) { return a_node.name; });

			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				if (!EditorHelper::IsMatchSearch(_search, _nodes[_i].name)) continue;

				bool _isSelected = (a_inoutNodeIndex == _i);

				// 同名でもImGuiのIDがぶつからないようにする
				ImGui::PushID(static_cast<int>(_i));

				const std::string _label = MakeUniqueLabel(
					_duplicatedSet, _nodes[_i].name, "#" + std::to_string(_i));

				if (ImGui::Selectable(_label.c_str(), _isSelected))
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

				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	bool ModelNodeField(
		const char* a_label,
		const Resource::Model* a_pModel,
		std::string& a_inoutNodeName,
		UINT& a_inoutNodeNameHash
	)
	{
		if (!a_pModel)
		{
			TextColored(WARNING_COLOR, "Warning: Model Resource is null.");
			return false;
		}

		const auto& _nodes = a_pModel->GetOriginalNodeVec();

		// 現在の選択表示
		std::string _currentNodeName = a_inoutNodeName.empty() ? "Select node..." : a_inoutNodeName;

		bool _isChanged = false;

		if (ImGui::BeginCombo(a_label, _currentNodeName.c_str()))
		{
			// ボーンは数が多いので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			// 同名のノードがあると一覧で見分けが付かないので、番号を添える対象を先に拾う。
			// ただしこの欄が持ち帰るのは名前(とそのハッシュ)なので、同名を選び分けても
			// 保存される値は同じになる。区別が要る場面ではインデックス版を使うこと
			const auto _duplicatedSet = CollectDuplicatedNames(
				_nodes, [](const auto& a_node) { return a_node.name; });

			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				const auto& _node = _nodes[_i];

				if (!EditorHelper::IsMatchSearch(_search, _node.name)) continue;

				bool _isSelected = (a_inoutNodeName == _node.name);

				// 同名でもImGuiのIDがぶつからないようにする
				ImGui::PushID(static_cast<int>(_i));

				const std::string _label = MakeUniqueLabel(
					_duplicatedSet, _node.name, "#" + std::to_string(_i));

				if (ImGui::Selectable(_label.c_str(), _isSelected))
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

				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	//--------------------------------------------------------------------------------------
	// モデルのアニメーション
	//--------------------------------------------------------------------------------------
	namespace
	{
		// アニメーション選択コンボの本体
		// ハンドルの持ち方(生ハンドル / 参照カウント付き)だけが違うので、選択結果だけを返す
		bool ModelAnimationFieldImpl(
			const ECS::EngineServices& a_services,
			const char* a_label,
			const Resource::Model* a_pModel,
			const Handle<Resource::AnimationData>& a_currentHandle,
			Handle<Resource::AnimationData>& a_outSelected
		)
		{
			if (!a_pModel) { return false; }

			const auto& _handleVec = a_pModel->GetAnimationHandles();

			// ハンドルから名前を引く。取れなければ空(この後の一覧では飛ばす)
			auto _animName = [&a_services](const auto& a_ref) -> std::string
			{
				const auto* _pAnim = a_services.pResourceManager->Get(a_ref);
				return _pAnim ? _pAnim->name : std::string();
			};

			// 現在の再生アニメ名をプレビューにする。
			// 同名のアニメを持つモデルもあるので、そのときは番号まで出す
			std::string _viewName = "Select...";
			for (size_t _i = 0; _i < _handleVec.size(); ++_i)
			{
				if (!(_handleVec[_i] == a_currentHandle)) continue;

				_viewName = _animName(_handleVec[_i]) + "   [#" + std::to_string(_i) + "]";
				break;
			}

			bool _isChanged = false;

			if (ImGui::BeginCombo(a_label, _viewName.c_str()))
			{
				// 名前で絞り込めるようにする
				const std::string& _search = EditorHelper::DrawSearchBox();

				// 同名のアニメは名前だけでは選び分けられないので、番号を添える対象を先に拾う
				const auto _duplicatedSet = CollectDuplicatedNames(_handleVec, _animName);

				for (size_t _i = 0; _i < _handleVec.size(); ++_i)
				{
					const auto& _ref = _handleVec[_i];

					const auto* _pAnim = a_services.pResourceManager->Get(_ref);
					if (!_pAnim) continue;

					if (!EditorHelper::IsMatchSearch(_search, _pAnim->name)) continue;

					bool _isSelected = (_ref == a_currentHandle);

					// 同名でもImGuiのIDがぶつからないようにする
					ImGui::PushID(static_cast<int>(_i));

					const std::string _label = MakeUniqueLabel(
						_duplicatedSet, _pAnim->name, "#" + std::to_string(_i));

					if (ImGui::Selectable(_label.c_str(), _isSelected))
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

					ImGui::PopID();
				}
				ImGui::EndCombo();
			}

			return _isChanged;
		}
	}

	bool ModelAnimationField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const Resource::Model* a_pModel,
		Handle<Resource::AnimationData>& a_inoutHandle
	)
	{
		Handle<Resource::AnimationData> _selected = {};
		if (!ModelAnimationFieldImpl(a_services, a_label, a_pModel, a_inoutHandle, _selected))
		{
			return false;
		}

		a_inoutHandle = _selected;
		return true;
	}

	bool ModelAnimationField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const Resource::Model* a_pModel,
		ResourceRef<Resource::AnimationData>& a_inoutRef
	)
	{
		Handle<Resource::AnimationData> _selected = {};
		if (!ModelAnimationFieldImpl(a_services, a_label, a_pModel, a_inoutRef.GetRaw(), _selected))
		{
			return false;
		}

		// 参照カウントの付け替えはResourceRef側に任せる
		a_inoutRef = ResourceRef<Resource::AnimationData>(_selected);
		return true;
	}

	//======================================================================================
	// 入力の状態
	//======================================================================================
	bool IsCtrlDown()
	{
		return ImGui::GetIO().KeyCtrl;
	}

	bool IsTextInputActive()
	{
		// WantCaptureKeyboard は NavEnableKeyboard 有効時、ImGuiウィンドウに
		// フォーカスがあるだけで常時 true になり得る(ドッキング型エディタでは
		// ほぼ常時になる)。テキスト入力中だけ true になる WantTextInput を見る
		if (ImGui::GetCurrentContext() == nullptr) return false;
		return ImGui::GetIO().WantTextInput;
	}

	//======================================================================================
	// シーンビュー上の操作
	//======================================================================================
	bool ScreenHandle(const char* a_id, const Math::Vector2& a_screenPos, float a_radius, Math::Vector2& a_outDragPos)
	{
		const ImVec2 _center = ToImVec2(a_screenPos);

		// ドラッグ操作用の透明ボタン
		ImGui::SetCursorScreenPos(ImVec2(_center.x - a_radius, _center.y - a_radius));
		ImGui::InvisibleButton(a_id, ImVec2(a_radius * 2.0f, a_radius * 2.0f));
		const bool _isActive = ImGui::IsItemActive();		// 掴まれているか
		const bool _isHovered = ImGui::IsItemHovered();		// カーソルが重なっているか

		// ハンドル描画(十字 + 円)
		ImDrawList* _pDrawList = ImGui::GetWindowDrawList();
		const ImU32 _color = _isActive ? IM_COL32(255, 200, 0, 255)
			: _isHovered ? IM_COL32(255, 255, 255, 255)
			: IM_COL32(0, 200, 255, 255);
		const float _crossLength = a_radius * 1.8f;
		_pDrawList->AddCircle(_center, a_radius, _color, 20, 2.0f);
		_pDrawList->AddLine(ImVec2(_center.x - _crossLength, _center.y), ImVec2(_center.x + _crossLength, _center.y), _color, 1.5f);
		_pDrawList->AddLine(ImVec2(_center.x, _center.y - _crossLength), ImVec2(_center.x, _center.y + _crossLength), _color, 1.5f);

		if (!_isActive) return false;

		const ImVec2 _mouse = ImGui::GetMousePos();
		a_outDragPos = Math::Vector2(_mouse.x, _mouse.y);
		return true;
	}

	bool TranslateGizmo(const Math::Matrix& a_viewMat, const Math::Matrix& a_projMat, Math::Matrix& a_inoutMat, float a_snap)
	{
		const float _snapValues[3] = { a_snap, a_snap, a_snap };

		ImGuizmo::Manipulate(
			&a_viewMat._11,
			&a_projMat._11,
			ImGuizmo::OPERATION::TRANSLATE,
			ImGuizmo::MODE::WORLD,
			&a_inoutMat._11,
			nullptr,
			(a_snap > 0.0f) ? _snapValues : nullptr
		);

		return ImGuizmo::IsUsing();
	}

	//======================================================================================
	// ノードエディタ
	//======================================================================================
	void NodeLink(int a_linkID, int a_srcOutPinID, int a_dstInPinID)
	{
		ImNodes::Link(a_linkID, a_srcOutPinID, a_dstInPinID);
	}

	//======================================================================================
	// スコープ
	//======================================================================================
	IDScope::IDScope(int a_id)			{ ImGui::PushID(a_id); }
	IDScope::IDScope(const char* a_id)	{ ImGui::PushID(a_id); }
	IDScope::IDScope(const void* a_id)	{ ImGui::PushID(a_id); }
	IDScope::~IDScope()					{ ImGui::PopID(); }

	DisabledScope::DisabledScope(bool a_isDisabled)	{ ImGui::BeginDisabled(a_isDisabled); }
	DisabledScope::~DisabledScope()					{ ImGui::EndDisabled(); }

	IndentScope::IndentScope()	{ ImGui::Indent(); }
	IndentScope::~IndentScope()	{ ImGui::Unindent(); }

	ItemWidthScope::ItemWidthScope(float a_width)	{ ImGui::PushItemWidth(a_width); }
	ItemWidthScope::~ItemWidthScope()				{ ImGui::PopItemWidth(); }

	TreeScope::TreeScope(const char* a_label, bool a_isDefaultOpen, bool a_isSpanFullWidth)
	{
		ImGuiTreeNodeFlags _flags = ImGuiTreeNodeFlags_None;
		if (a_isDefaultOpen)	_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (a_isSpanFullWidth)	_flags |= ImGuiTreeNodeFlags_SpanFullWidth;

		m_isOpen = ImGui::TreeNodeEx(a_label, _flags);
	}
	TreeScope::~TreeScope()
	{
		// 閉じているときは TreePop しない(開いたときだけ積まれる)
		if (m_isOpen) ImGui::TreePop();
	}

	ComboScope::ComboScope(const char* a_label, const char* a_preview)
	{
		m_isOpen = ImGui::BeginCombo(a_label, a_preview);
	}
	ComboScope::~ComboScope()
	{
		// 開いたときだけ閉じる
		if (m_isOpen) ImGui::EndCombo();
	}

	WindowScope::WindowScope(const char* a_name)
	{
		m_isVisible = ImGui::Begin(a_name);
	}
	WindowScope::~WindowScope()
	{
		// Begin は結果に関わらず End と対にする
		ImGui::End();
	}
}
