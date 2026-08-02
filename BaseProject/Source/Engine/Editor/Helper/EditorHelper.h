#pragma once

//------------------------------------------------------------------------------------------
// このヘッダーはAnimatorAsset -> StateGraphEditor 経由で
// ResourceManagerより先に読まれるため、重いヘッダーは持ち込まない。
// 実装で必要な型は EditorHelper.cpp 側でインクルードする。
//------------------------------------------------------------------------------------------
namespace Engine::Resource
{
	class Model;
	class Texture;
	struct AnimationData;
}

namespace Engine::Editor
{
	// 型安全に値を参照する
	// コンポーネントのArchive / Editが受け取るvoid*を実体に戻すためのもの
	template<typename T>
	T& GetValue(void* a_data)
	{
		return *reinterpret_cast<T*>(a_data);
	}

	/// <summary>
	/// 行列の表示形式
	/// </summary>
	enum class EMatrixViewMode
	{
		Raw,			// 4x4の成分をそのまま並べる
		PosRotScale,	// 位置・回転(度)・スケールに分解して表示する
	};

	//==========================================================================================
	//
	// エディター上で使い回す描画ヘルパー
	// 「アセット選択」「モデル内の選択」「Enum選択」「テクスチャ表示」
	// 「行列/ハンドル表示」「ノードエディタ部品」を1か所にまとめている
	//
	//==========================================================================================
	class EditorHelper
	{
	public:
		//--------------------------------------------------------------------------------------
		// アセット選択
		//--------------------------------------------------------------------------------------

		/// <summary>
		/// アセットデータベースから1件選ばせるだけの土台
		/// GUIDもハンドルもこちらでは書き換えないので、
		/// 反映方法が特殊なもの(独自のロード関数を通す等)はこれを直接使う
		/// </summary>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_assetTypeName">アセットデータベースに渡す型名</param>
		/// <param name="a_currentGUID">現在選択中のGUID(表示と選択中判定に使う)</param>
		/// <param name="a_outSelectedGUID">選択されたGUIDの受け取り先</param>
		/// <returns>選択されたら true</returns>
		static bool DrawAssetGUIDCombo(
			const char* a_lable,
			const char* a_assetTypeName,
			const GUID& a_currentGUID,
			GUID& a_outSelectedGUID
		);

		/// <summary>
		/// GUIDのみを書き換えるアセット検索欄
		/// ハンドルはここでは書き換えないため、実体の差し替えを
		/// リフレッシュ経路（Release → PostDeserializeでのFixup）に任せたい場合に使う
		/// </summary>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_assetTypeName">アセットデータベースに渡す型名</param>
		/// <param name="a_inoutGUID">上書きされるGUID</param>
		/// <returns>選択が変更されたら true</returns>
		static bool DrawAssetSelectComboGUID(
			const char* a_lable,
			const char* a_assetTypeName,
			GUID& a_inoutGUID
		);

		/// <summary>
		/// リソースマネージャーから特定の種類のアセットの検索欄を作成
		/// 選択された時点でロードまで済ませ、GUIDとハンドルの両方を更新する
		/// </summary>
		/// <typeparam name="TResource">リソースの型</typeparam>
		/// <typeparam name="THandle">リソースの型のハンドル</typeparam>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_assetTypeName">アセットデータベースに渡す型名</param>
		/// <param name="a_inoutGUID">上書きされるGUID</param>
		/// <param name="a_inoutHandle">上書きされるハンドル</param>
		/// <returns>選択が変更されたら true</returns>
		/// <remarks>
		/// 実装はResourceManagerを触るため EditorHelper.inl にある。
		/// この関数を使う側は EditorHelper.inl をインクルードすること。
		/// </remarks>
		template<typename TResource, typename THandle>
		static bool DrawAssetSelectCombo(
			const char* a_lable,
			const char* a_assetTypeName,
			Engine::GUID& a_inoutGUID,
			THandle& a_inoutHandle
		);

		//--------------------------------------------------------------------------------------
		// モデルが持つデータの選択
		//--------------------------------------------------------------------------------------

		/// <summary>
		/// モデルのノードをインデックスで選択する
		/// </summary>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_pModel">対象のモデル</param>
		/// <param name="a_inoutNodeIndex">上書きされるノードインデックス</param>
		/// <param name="a_inoutNodeNameHash">上書きされるノード名ハッシュ</param>
		/// <returns>選択が変更されたら true</returns>
		static bool DrawModelNodeCombo(
			const char* a_lable,
			const Resource::Model* a_pModel,
			UINT& a_inoutNodeIndex,
			UINT& a_inoutNodeNameHash
		);

		/// <summary>
		/// モデルのノードを名前で選択する
		/// ノードの並びが変わっても壊れないよう、名前で持ちたい側が使う
		/// </summary>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_pModel">対象のモデル</param>
		/// <param name="a_inoutNodeName">上書きされるノード名</param>
		/// <param name="a_inoutNodeNameHash">上書きされるノード名ハッシュ</param>
		/// <returns>選択が変更されたら true</returns>
		static bool DrawModelNodeComboByName(
			const char* a_lable,
			const Resource::Model* a_pModel,
			std::string& a_inoutNodeName,
			UINT& a_inoutNodeNameHash
		);

		/// <summary>
		/// モデルが持つアニメーションを選択する
		/// </summary>
		/// <param name="a_lable">コンボボックスのラベル</param>
		/// <param name="a_pModel">対象のモデル</param>
		/// <param name="a_inoutHandle">上書きされるアニメーションハンドル</param>
		/// <returns>選択が変更されたら true</returns>
		static bool DrawModelAnimationCombo(
			const char* a_lable,
			const Resource::Model* a_pModel,
			Handle<Resource::AnimationData>& a_inoutHandle
		);

		/// <summary>
		/// モデルが持つアニメーションを選択する(参照カウント付きハンドル版)
		/// </summary>
		static bool DrawModelAnimationCombo(
			const char* a_lable,
			const Resource::Model* a_pModel,
			ResourceRef<Resource::AnimationData>& a_inoutRef
		);

		//--------------------------------------------------------------------------------------
		// Enum
		//--------------------------------------------------------------------------------------

		/// <summary>
		/// Enumをコンボとして描画する
		/// </summary>
		template<typename Enum>
		static bool DrawEnumCombo(const char* a_lable, Enum& a_value)
		{
			const char* _preview = magic_enum::enum_name(a_value).data();

			bool _isChange = false;

			if (ImGui::BeginCombo(a_lable, _preview))
			{
				for (auto _v : magic_enum::enum_values<Enum>())
				{
					bool _isSelect = (_v == a_value);

					if (ImGui::Selectable(magic_enum::enum_name(_v).data(), _isSelect))
					{
						a_value = _v;
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

		/// <summary>
		/// Enumをビットのコンボとして描画する
		/// </summary>
		template<typename Enum>
		static void DrawEnumFlagsCombo(const char* a_lable, Enum& a_value)
		{
			using U = std::underlying_type_t<Enum>;
			U _raw = static_cast<U>(a_value);

			std::string _preview = "None";

			if (_raw != 0)
			{
				_preview.clear();
				for (auto _v : magic_enum::enum_values<Enum>())
				{
					U _bit = static_cast<U>(_v);
					if (_bit != 0 && (_raw & _bit))
					{
						if (!_preview.empty())
						{
							_preview += " | ";
						}
						_preview += magic_enum::enum_name(_v);
					}
				}
			}

			if (ImGui::BeginCombo(a_lable, _preview.c_str()))
			{
				for (auto _v : magic_enum::enum_values<Enum>())
				{
					if (_v == Enum{}) continue;		// None除外するなら

					U _bit = static_cast<U>(_v);
					bool _isCheck = (_raw & _bit) != 0;

					if (ImGui::Selectable(magic_enum::enum_name(_v).data(), _isCheck))
					{
						if (_isCheck)
						{
							_raw &= ~_bit;
						}
						else
						{
							_raw |= _bit;
						}
					}
				}
				a_value = static_cast<Enum>(_raw);
				ImGui::EndCombo();
			}
		}

		//--------------------------------------------------------------------------------------
		// テクスチャ
		//--------------------------------------------------------------------------------------

		/// <summary>
		/// エディター上でテクスチャを表示する
		/// </summary>
		/// <param name="a_handle">テクスチャのハンドル</param>
		/// <param name="a_width">横幅</param>
		/// <param name="a_height">縦</param>
		/// <returns>実際に描画したサイズ</returns>
		static ImVec2 DrawTexture(
			const Handle<Resource::Texture>& a_handle,
			float a_width = 0,
			float a_height = 0
		);

		/// <summary>
		/// SRVを画像としてImGui上に描画させる
		/// </summary>
		/// <param name="a_gpuHandle">GPUハンドル</param>
		/// <param name="a_width">横幅</param>
		/// <param name="a_height">縦</param>
		/// <returns>実際に描画した範囲</returns>
		static ImVec2 DrawSRVView(
			D3D12_GPU_DESCRIPTOR_HANDLE a_gpuHandle,
			float a_width, float a_height,
			float a_minSize = 100, float a_maxSize = 500
		);

		//--------------------------------------------------------------------------------------
		// 行列・回転
		//--------------------------------------------------------------------------------------

		/// <summary>
		/// 行列の編集
		/// 「成分そのまま」と「位置・回転・スケール」を切り替えて表示できる。
		/// どちらを出すかはラベルごとにImGui側へ覚えさせるので、呼ぶ側は何も持たなくてよい。
		/// </summary>
		/// <param name="a_lable">項目名(ImGuiのID兼用なので呼び出しごとに変える)</param>
		/// <param name="a_mat">編集する行列</param>
		/// <param name="a_defaultMode">初回に表示する形式</param>
		/// <returns>値が編集されたら true</returns>
		static bool DrawMatrix(
			const char* a_lable,
			DirectX::XMFLOAT4X4& a_mat,
			EMatrixViewMode a_defaultMode = EMatrixViewMode::Raw
		);

		/// <summary>
		/// クォータニオンを度数法のオイラー角として編集する
		/// </summary>
		/// <param name="a_quat">編集するクォータニオン</param>
		/// <returns>値が編集されたら true</returns>
		static bool DragRotationDeg3FromQuaternion(DirectX::XMFLOAT4& a_quat);

		//--------------------------------------------------------------------------------------
		// ハンドル
		//--------------------------------------------------------------------------------------

		template<typename T>
		static void DrawHandle(const Handle<T>& a_handle)
		{
			ImGui::Text("Handle : id = %d", static_cast<int>(a_handle.id));
			ImGui::Text("index = %d", static_cast<int>(a_handle.GetIndex()));
			ImGui::Text("generation = %d", static_cast<int>(a_handle.GetGeneration()));
		}

		template<typename T>
		static void DrawHandle(const RangeHandle<T>& a_handle)
		{
			ImGui::Text("Handle : generation = %d", static_cast<int>(a_handle.generation));
			ImGui::Text("startIndex = %d", static_cast<int>(a_handle.startIndex));
			ImGui::Text("count = %d", static_cast<int>(a_handle.count));
		}

		//--------------------------------------------------------------------------------------
		// ノードエディタ部品
		//--------------------------------------------------------------------------------------

		// ノードのタイトルバー表示
		static void DrawNodeTitleBar(const std::string& a_name);
	};
}
