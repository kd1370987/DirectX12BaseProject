#pragma once

//------------------------------------------------------------------------------------------
// エディターの外(コンポーネントの Edit・ゲームオブジェクト・オプション・アセットなど)から
// 編集UIを組むための入口。
//
// ImGui をエディターディレクトリの外へ漏らさないためのもので、
// 外側はここの関数だけで編集UIを組む(ImGui:: を直接書かない)。
// そのためこのヘッダーは ImGui を知らず、引数には自前の数学型を使う。
// 実装(EditorField.cpp)だけが ImGui を触る。
//
// EditorHelper.h と同じく AnimatorAsset 経由で ResourceManager より先に読まれるため、
// 重いヘッダーは持ち込まない。ロードまで行うアセット欄は EditorField.inl にある。
//------------------------------------------------------------------------------------------
namespace Engine::Resource
{
	class Model;
	class Texture;
	struct AnimationData;
}

namespace Engine::ECS
{
	struct EngineServices;
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

	/// <summary>
	/// 矢印ボタンの向き
	/// </summary>
	enum class EArrowDir
	{
		Left,
		Right,
		Up,
		Down,
	};

	//======================================================================================
	// 文字
	//======================================================================================

	// 文字列(printf 形式)
	void Text(const char* a_fmt, ...);

	// 説明文 : 薄い色で出す
	void HelpText(const char* a_fmt, ...);

	// 色付きの文字 : 警告やエラーを目立たせる
	void TextColored(const Math::Color& a_color, const char* a_fmt, ...);

	// 右にラベルを添えた文字 : 編集できない値を Field と同じ並びで見せる
	void LabelText(const char* a_label, const char* a_fmt, ...);

	// 行頭に点を付けた文字
	void BulletText(const char* a_fmt, ...);

	// 直前の項目にカーソルが乗っている間だけ出す説明
	void Tooltip(const char* a_fmt, ...);

	//======================================================================================
	// 区切り・配置
	//======================================================================================

	// 前後に余白を取った区切り線
	void Line();

	// 区切り線だけ
	void Separator();

	// 見出し付きの区切り線
	void Section(const char* a_label);

	// 余白
	void Spacing();

	// 次の項目を同じ行へ置く
	void SameLine();

	// 次の項目1つの横幅
	void SetNextItemWidth(float a_width);

	//======================================================================================
	// 値の変更 : 値が変わったら true を返す
	//
	// 数値のドラッグは speed / min / max を省略できる。min と max が両方 0 なら範囲なし
	//======================================================================================

	// 真偽(チェックボックス)
	bool Field(const char* a_label, bool& a_value);

	// 整数
	bool Field(const char* a_label, int& a_value, float a_speed = 1.0f, int a_min = 0, int a_max = 0);
	bool Field(const char* a_label, int (&a_values)[2], float a_speed = 1.0f, int a_min = 0, int a_max = 0);

	// 符号なし整数 : 打ち込みで編集する(ドラッグでは大きな値を扱いにくいため)
	bool Field(const char* a_label, uint32_t& a_value);
	bool Field(const char* a_label, uint64_t& a_value);

	// 小数 : a_format は表示の書式("%.2f s" など)。nullptr なら既定
	bool Field(const char* a_label, float& a_value, float a_speed = 1.0f, float a_min = 0.0f, float a_max = 0.0f, const char* a_format = nullptr);
	bool Field(const char* a_label, Math::Vector2& a_value, float a_speed = 1.0f, float a_min = 0.0f, float a_max = 0.0f, const char* a_format = nullptr);
	bool Field(const char* a_label, Math::Vector3& a_value, float a_speed = 1.0f, float a_min = 0.0f, float a_max = 0.0f, const char* a_format = nullptr);

	// 回転 : 度数法のオイラー角として編集する
	bool Field(const char* a_label, Math::Quaternion& a_value);

	/// <summary>
	/// 色(RGBA)を編集する。UI/HUD の色はすべてこれを通す
	/// </summary>
	/// <remarks>
	/// スウォッチとピッカーに加えて、下に数値のドラッグを出す。
	/// UI は HDR のレンダーターゲット(AfterLighting)へ描くので 1 を超える値も
	/// 意味がある(光らせたい枠など)が、ピッカーは 0..1 しか触れないため。
	/// </remarks>
	bool Field(const char* a_label, Math::Color& a_value);

	/// <summary>
	/// 行列の編集
	/// 「成分そのまま」と「位置・回転・スケール」を切り替えて表示できる。
	/// どちらを出すかはラベルごとに覚えるので、呼ぶ側は何も持たなくてよい。
	/// </summary>
	/// <param name="a_label">項目名(IDも兼ねるので呼び出しごとに変える)</param>
	/// <param name="a_defaultMode">初回に表示する形式</param>
	bool Field(const char* a_label, Math::Matrix& a_value, EMatrixViewMode a_defaultMode = EMatrixViewMode::Raw);

	// 文字列
	bool Field(const char* a_label, std::string& a_value);
	bool Field(const char* a_label, char* a_buff, size_t a_buffSize);

	// 複数行の文字列
	bool MultilineField(const char* a_label, std::string& a_value);

	// スライダー : 範囲が決まっている値(音量・割合など)
	bool Slider(const char* a_label, float& a_value, float a_min, float a_max);

	// 範囲(最小と最大の2つ) : a_lowerLimit / a_upperLimit は両方の動かせる幅
	bool RangeField(const char* a_label, float& a_min, float& a_max, float a_speed, float a_lowerLimit, float a_upperLimit);

	// 色見本(クリックでピッカー) : Field(Color) と違い 0..1 の範囲だけを扱う
	bool ColorField(const char* a_label, Math::Color& a_value, bool a_isAlpha = true);
	bool ColorField(const char* a_label, Math::Vector3& a_rgb);

	// 常に開いたままのカラーピッカー
	bool ColorPicker(const char* a_label, Math::Color& a_value);

	// 文字列の並びから1つ選ぶ
	bool Combo(const char* a_label, int& a_index, const char* const a_items[], int a_count);
	template<size_t N>
	bool Combo(const char* a_label, int& a_index, const char* const (&a_items)[N])
	{
		return Combo(a_label, a_index, a_items, static_cast<int>(N));
	}

	// 直前の項目の編集が終わったか(ドラッグを離した・入力を確定した瞬間だけ true)
	// 触っている間ずっと保存したくないときに使う
	bool IsItemEditFinished();

	//--------------------------------------------------------------------------------------
	// Enum
	//--------------------------------------------------------------------------------------
	namespace Internal
	{
		// Enum の選択の本体 : a_index は a_names の番号(範囲外なら未選択)
		bool EnumCombo(const char* a_label, size_t& a_index, std::span<const std::string_view> a_names);

		// ビットフラグの選択の本体
		bool FlagsCombo(const char* a_label, uint64_t& a_raw,
			std::span<const uint64_t> a_bits, std::span<const std::string_view> a_names);
	}

	/// <summary>
	/// Enumをコンボとして編集する
	/// </summary>
	template<typename Enum>
		requires std::is_enum_v<Enum>
	bool Field(const char* a_label, Enum& a_value)
	{
		constexpr auto _values = magic_enum::enum_values<Enum>();
		constexpr auto _names = magic_enum::enum_names<Enum>();

		size_t _index = magic_enum::enum_index(a_value).value_or(_values.size());
		if (!Internal::EnumCombo(a_label, _index, _names)) return false;

		a_value = _values[_index];
		return true;
	}

	/// <summary>
	/// Enumをビットフラグのコンボとして編集する
	/// 値が 0 の項目(None など)は選択肢に出さない
	/// </summary>
	template<typename Enum>
		requires std::is_enum_v<Enum>
	bool FlagsField(const char* a_label, Enum& a_value)
	{
		using U = std::underlying_type_t<Enum>;

		constexpr auto _values = magic_enum::enum_values<Enum>();
		constexpr auto _names = magic_enum::enum_names<Enum>();

		std::array<uint64_t, _values.size()> _bits = {};
		for (size_t _i = 0; _i < _values.size(); ++_i)
		{
			_bits[_i] = static_cast<uint64_t>(static_cast<U>(_values[_i]));
		}

		uint64_t _raw = static_cast<uint64_t>(static_cast<U>(a_value));
		if (!Internal::FlagsCombo(a_label, _raw, _bits, _names)) return false;

		a_value = static_cast<Enum>(static_cast<U>(_raw));
		return true;
	}

	//======================================================================================
	// ボタン・選択 : 押された(選ばれた)ら true
	//======================================================================================

	// 通常のボタン : 保存・リセット・表示切り替えなど
	bool Button(const char* a_label, const Math::Vector2& a_size = Math::Vector2(0.0f, 0.0f));
	bool SmallButton(const char* a_label);
	bool ArrowButton(const char* a_id, EArrowDir a_dir);

	//--------------------------------------------------------------------------------------
	// 押した先の結果で色を分けたボタン
	//   生成(増える)  … 緑
	//   削除(減る)    … 赤
	// 色だけで「押すと何が起きるか」が分かるので、並んだボタンを読み違えにくくなる。
	// それ以外は通常色の Button を使う。
	//--------------------------------------------------------------------------------------
	bool CreateButton(const char* a_label, const Math::Vector2& a_size = Math::Vector2(0.0f, 0.0f));
	bool CreateSmallButton(const char* a_label);
	bool DeleteButton(const char* a_label, const Math::Vector2& a_size = Math::Vector2(0.0f, 0.0f));
	bool DeleteSmallButton(const char* a_label);

	// ラジオボタン : a_isActive は今選ばれているか
	bool RadioButton(const char* a_label, bool a_isActive);

	// 一覧の1行
	bool Selectable(const char* a_label, bool a_isSelected);

	// コンボを開いたときに、直前の項目までスクロールしてフォーカスを当てる
	void SetItemDefaultFocus();

	// 折りたたみ見出し : 開いていたら true
	bool CollapsingHeader(const char* a_label, bool a_isDefaultOpen = false);

	//======================================================================================
	// 表示
	//======================================================================================

	// 進捗バー : 横幅いっぱいに出す。a_overlay は中に出す文字(nullptr なら割合)
	void ProgressBar(float a_fraction, const char* a_overlay = nullptr);

	/// <summary>
	/// テクスチャを表示する : 横幅いっぱいに、a_width : a_height の比率で出す
	/// </summary>
	/// <param name="a_services">テクスチャの実体を引く先</param>
	/// <returns>実際に描画したサイズ</returns>
	Math::Vector2 Image(
		const ECS::EngineServices& a_services,
		const Handle<Resource::Texture>& a_handle,
		float a_width = 0,
		float a_height = 0
	);

	// ハンドルの中身
	template<typename T>
	void HandleInfo(const Handle<T>& a_handle)
	{
		Text("Handle : id = %d", static_cast<int>(a_handle.id));
		Text("index = %d", static_cast<int>(a_handle.GetIndex()));
		Text("generation = %d", static_cast<int>(a_handle.GetGeneration()));
	}

	template<typename T>
	void HandleInfo(const RangeHandle<T>& a_handle)
	{
		Text("Handle : generation = %d", static_cast<int>(a_handle.generation));
		Text("startIndex = %d", static_cast<int>(a_handle.startIndex));
		Text("count = %d", static_cast<int>(a_handle.count));
	}

	//======================================================================================
	// アセット選択
	//
	// どれも先頭で EngineServices を受け取る。
	// アセット一覧(AssetDatabase)とロード(ResourceManager)をそこから引くため。
	//   コンポーネントの Edit      : *a_context.pWorld->RefEngineServices()
	//   ゲームオブジェクト         : *a_context.pServices
	//   エディターのパネル         : *a_editContext.pServices
	//======================================================================================

	/// <summary>
	/// アセットデータベースから1件選ばせるだけの土台
	/// GUIDもハンドルもこちらでは書き換えないので、
	/// 反映方法が特殊なもの(独自のロード関数を通す等)はこれを直接使う
	/// </summary>
	/// <param name="a_assetTypeName">アセットデータベースに渡す型名</param>
	/// <param name="a_currentGUID">現在選択中のGUID(表示と選択中判定に使う)</param>
	/// <param name="a_outSelectedGUID">選択されたGUIDの受け取り先</param>
	/// <returns>選択されたら true</returns>
	bool AssetPicker(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		const GUID& a_currentGUID,
		GUID& a_outSelectedGUID
	);

	/// <summary>
	/// GUIDのみを書き換えるアセット欄
	/// ハンドルはここでは書き換えないため、実体の差し替えを
	/// リフレッシュ経路（Release → PostDeserializeでのFixup）に任せたい場合に使う
	/// </summary>
	/// <returns>選択が変更されたら true</returns>
	bool AssetField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		GUID& a_inoutGUID
	);

	/// <summary>
	/// 選ばれた時点でロードまで済ませ、GUIDとハンドルの両方を更新するアセット欄
	/// </summary>
	/// <remarks>
	/// 実装はResourceManagerを触るため EditorField.inl にある。
	/// この関数を使う側は EditorField.inl をインクルードすること。
	/// </remarks>
	template<typename TResource, typename THandle>
	bool AssetField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		GUID& a_inoutGUID,
		THandle& a_inoutHandle
	);

	//--------------------------------------------------------------------------------------
	// モデルが持つデータの選択
	//--------------------------------------------------------------------------------------

	// モデルのノードをインデックスで選択する
	bool ModelNodeField(
		const char* a_label,
		const Resource::Model* a_pModel,
		UINT& a_inoutNodeIndex,
		UINT& a_inoutNodeNameHash
	);

	// モデルのノードを名前で選択する
	// ノードの並びが変わっても壊れないよう、名前で持ちたい側が使う
	bool ModelNodeField(
		const char* a_label,
		const Resource::Model* a_pModel,
		std::string& a_inoutNodeName,
		UINT& a_inoutNodeNameHash
	);

	// モデルが持つアニメーションを選択する
	bool ModelAnimationField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const Resource::Model* a_pModel,
		Handle<Resource::AnimationData>& a_inoutHandle
	);

	// モデルが持つアニメーションを選択する(参照カウント付きハンドル版)
	bool ModelAnimationField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const Resource::Model* a_pModel,
		ResourceRef<Resource::AnimationData>& a_inoutRef
	);

	//======================================================================================
	// 名前の衝突対策 (文字列を組むだけで、UIは出さない)
	//======================================================================================

	/// <summary>
	/// 一覧の中で2件以上ある名前を集める
	/// </summary>
	/// <param name="a_range">候補の並び</param>
	/// <param name="a_getName">1件から表示名を取り出す関数</param>
	/// <returns>重複していた名前の集合</returns>
	/// <remarks>
	/// コンボの選択欄は名前しか出さないので、同名のアセット・同名のボーンが並ぶと
	/// どちらを選んでいるのか分からなくなる。重複しているものにだけ手掛かり
	/// (置き場所や番号)を添えるために、先に重複を数えておく。
	/// 全件に手掛かりを付けると普段の一覧がうるさくなるので、必要なものだけに絞る。
	/// </remarks>
	template<typename Range, typename GetName>
	std::unordered_set<std::string> CollectDuplicatedNames(const Range& a_range, GetName a_getName)
	{
		std::unordered_map<std::string, int> _countMap;
		for (const auto& _item : a_range) ++_countMap[a_getName(_item)];

		std::unordered_set<std::string> _duplicatedSet;
		for (const auto& [_name, _count] : _countMap)
		{
			if (_count > 1) _duplicatedSet.insert(_name);
		}

		return _duplicatedSet;
	}

	/// <summary>
	/// 表示名に手掛かりを足す(重複していないときはそのまま)
	/// </summary>
	/// <param name="a_duplicatedSet">CollectDuplicatedNames の結果</param>
	/// <param name="a_name">候補の表示名</param>
	/// <param name="a_hint">同名があるときに添える文字(置き場所・番号など)</param>
	inline std::string MakeUniqueLabel(
		const std::unordered_set<std::string>& a_duplicatedSet,
		const std::string& a_name,
		const std::string& a_hint)
	{
		if (a_duplicatedSet.find(a_name) == a_duplicatedSet.end()) return a_name;
		if (a_hint.empty()) return a_name;

		return a_name + "   [" + a_hint + "]";
	}

	//======================================================================================
	// 入力の状態
	//======================================================================================

	// Ctrl を押しているか : 「押しながらで確定」「押しながらでスナップ」に使う
	bool IsCtrlDown();

	// エディターの文字入力欄にフォーカスがあるか(=文字入力中か)
	// エディターが無い(コンテキストが無い)ときは false
	bool IsTextInputActive();

	//======================================================================================
	// シーンビュー上の操作 (ObjectGizmoContext を受け取る DrawGizmo の中で使う)
	//======================================================================================

	/// <summary>
	/// 画面上の1点をドラッグで動かすためのハンドル(円 + 十字)
	/// </summary>
	/// <param name="a_id">ID(同じ窓に複数置くなら変える)</param>
	/// <param name="a_screenPos">ハンドルの位置(スクリーン絶対座標, px)</param>
	/// <param name="a_radius">ハンドルの半径(px)</param>
	/// <param name="a_outDragPos">ドラッグ中のカーソル位置(スクリーン絶対座標, px)</param>
	/// <returns>ドラッグ中なら true(このときだけ a_outDragPos が書かれる)</returns>
	bool ScreenHandle(const char* a_id, const Math::Vector2& a_screenPos, float a_radius, Math::Vector2& a_outDragPos);

	/// <summary>
	/// 行列の位置をワールド軸の移動ギズモで動かす
	/// </summary>
	/// <param name="a_snap">スナップの刻み。0 ならスナップしない</param>
	/// <returns>ギズモを操作中なら true(このとき a_inoutMat が書き換わっている)</returns>
	bool TranslateGizmo(const Math::Matrix& a_viewMat, const Math::Matrix& a_projMat, Math::Matrix& a_inoutMat, float a_snap = 0.0f);

	//======================================================================================
	// ノードエディタ
	//======================================================================================

	// ノード間の線を登録する : ノードエディタの描画中に呼ぶ
	void NodeLink(int a_linkID, int a_srcOutPinID, int a_dstInPinID);

	//======================================================================================
	// スコープ(RAII)
	//
	// 対になる命令(開始 / 終了)はスコープで持つ。抜けたときに閉じるので閉じ忘れが起きない。
	// 開いたかどうかで中身を出し分けるものは、if の条件にそのまま書ける
	//   if (Engine::Editor::TreeScope _tree{ "Detail" }) { ... }
	//======================================================================================

	// ID をずらす : 同じラベルの項目を並べるときに使う
	class IDScope
	{
	public:
		explicit IDScope(int a_id);
		explicit IDScope(const char* a_id);
		explicit IDScope(const void* a_id);
		~IDScope();
		NON_COPYABLE_NON_MOVABLE(IDScope);
	};

	// 中の項目を触れなくする(灰色で出す)
	class DisabledScope
	{
	public:
		explicit DisabledScope(bool a_isDisabled = true);
		~DisabledScope();
		NON_COPYABLE_NON_MOVABLE(DisabledScope);
	};

	// 中の項目を一段下げる
	class IndentScope
	{
	public:
		IndentScope();
		~IndentScope();
		NON_COPYABLE_NON_MOVABLE(IndentScope);
	};

	// 中の項目の横幅
	class ItemWidthScope
	{
	public:
		explicit ItemWidthScope(float a_width);
		~ItemWidthScope();
		NON_COPYABLE_NON_MOVABLE(ItemWidthScope);
	};

	// 開閉できるツリー : 開いていたら true
	class TreeScope
	{
	public:
		explicit TreeScope(const char* a_label, bool a_isDefaultOpen = false, bool a_isSpanFullWidth = false);
		~TreeScope();
		NON_COPYABLE_NON_MOVABLE(TreeScope);

		explicit operator bool() const { return m_isOpen; }

	private:
		bool m_isOpen = false;
	};

	// 自分で中身を並べるコンボ : 開いていたら true
	// 中身は Selectable と SetItemDefaultFocus で並べる
	class ComboScope
	{
	public:
		ComboScope(const char* a_label, const char* a_preview);
		~ComboScope();
		NON_COPYABLE_NON_MOVABLE(ComboScope);

		explicit operator bool() const { return m_isOpen; }

	private:
		bool m_isOpen = false;
	};

	// 独立したウィンドウ : 中身を出してよければ true
	// (閉じている・折りたたまれているときは false。どちらでも抜けるときに閉じる)
	class WindowScope
	{
	public:
		explicit WindowScope(const char* a_name);
		~WindowScope();
		NON_COPYABLE_NON_MOVABLE(WindowScope);

		explicit operator bool() const { return m_isVisible; }

	private:
		bool m_isVisible = false;
	};
}
