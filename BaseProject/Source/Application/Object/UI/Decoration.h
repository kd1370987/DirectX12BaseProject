#pragma once

//==========================================================================================
// UIのデコレーション
//
// UI 1つは「アンカー(位置・回転・倍率・色)」だけを持ち、実際に見えるものは
// このデコレーションを配列で生やして作る。
//
//   UIBase  … 画面のどこに、どの向き・どの大きさで置くか
//   Decoration … そこから相対でどんな絵を出すか(枠・画像・文字)
//
// 値はすべて親からの相対。親を動かせば飾りも一緒に動き、回せば一緒に回り、
// 倍率を掛ければオフセットごと伸びる。1枚のUIを小さい絵の重ね合わせで作れるようにするため。
//
// 種類は enum で分けた1つの struct にまとめてある(タグ付き)。
// 配列1本で持てるので、描画順が配列順そのままになり、
// インスペクターの追加・削除・並べ替えも Archive も1つのリストで済む。
//==========================================================================================
// 描画命令の積み先。ヘッダーでは前方宣言だけにして、実装側で GraphicEngine.h を読む
namespace Engine::Graphics { class GraphicsEngine; }

namespace App::Object::Decoration
{
	//======================================================================================
	// イージング
	//======================================================================================
	enum class EEase : uint32_t
	{
		Linear,			// 等速
		InQuad,			// ゆっくり始まる
		OutQuad,		// ゆっくり終わる
		InOutQuad,		// 両端がゆっくり
		InCubic,
		OutCubic,
		InOutCubic,
		OutBack,		// 行き過ぎて戻る
		OutElastic,		// 弾んで止まる
	};

	// 0〜1 の進み具合にイージングを掛ける
	float Ease(EEase a_ease, float a_rate);

	//======================================================================================
	// アニメーションで動かすチャンネル
	//
	// 「持っているだけで全部動く」形にすると、触っていないチャンネルまで
	// end(既定値 = 0)へ向かってしまい、指定した覚えのない縮小や透明化が起きる。
	// 動かすものをここで明示する
	//======================================================================================
	enum class EAnimChannel : uint32_t
	{
		NONE		= 0,
		COLOR		= 1 << 0,	// 色(乗算)
		POSITION	= 1 << 1,	// 位置(px, 加算)
		SCALE		= 1 << 2,	// 大きさ(倍率, 乗算)
		ROTATION	= 1 << 3,	// 回転(度, 加算)
		UV			= 1 << 4,	// UVオフセット(加算)

		ALL			= COLOR | POSITION | SCALE | ROTATION | UV,
	};
	ENUM_ATTR_BITFLAG(EAnimChannel);

	template<typename T>
	struct AnimElement
	{
		T start = {};
		T end = {};
	};

	//--------------------------------------------------------------------------------------
	// 周期運動1つぶん
	//
	// 既定は「中心から ±amplitude 振る」。中心は、足すチャンネル(位置・回転)なら 0、
	// 掛けるチャンネル(大きさ・色)なら 1 なので、振幅0で何も起きない。
	//
	// isUseLimit を立てると amplitude は使わず、minValue〜maxValue の間を往復する。
	// 「アルファは 0.3〜0.8 の間だけ。完全に不透明にはしない」のように、
	// 振れ幅ではなく届く範囲そのものを決めたいときはこちら
	//--------------------------------------------------------------------------------------
	template<typename T>
	struct Oscillation
	{
		T amplitude = {};
		float frequency = 1.0f;		// 1秒あたりの往復回数
		float phase = 0.0f;			// 位相のずれ(0〜1で1周)

		// 上下限で挟む
		bool isUseLimit = false;
		T minValue = {};			// 下限(波の底で届く値)
		T maxValue = {};			// 上限(波の頂点で届く値)
	};

	//--------------------------------------------------------------------------------------
	// 始点から終点へ1回だけ動かす(トゥイーン)
	//--------------------------------------------------------------------------------------
	struct UIAnimation
	{
		float currentTime = 0.0f;		// 経過時間
		float durationTime = 1.0f;		// 変化するのにかかる時間
		bool isLoop = false;			// ループ設定
		bool isPingPong = false;		// 往復するか(往きで end、復りで start へ戻る)

		EEase ease = EEase::Linear;					// 進み具合の曲線
		EAnimChannel channels = EAnimChannel::NONE;	// 動かすチャンネル

		// TweenAnimデータ : channels で選んだものだけが効く
		AnimElement<Math::Color> color = {};		// 元の色へ乗算
		AnimElement<Math::Vector2> position = {};	// 位置へ加算(px)
		AnimElement<Math::Vector2> scale = {};		// 大きさへ乗算(1で等倍)
		AnimElement<float> rotation = {};			// 回転へ加算(度)
		AnimElement<Math::Vector2> uv = {};			// UVオフセットへ加算
	};

	//--------------------------------------------------------------------------------------
	// ずっと揺らし続ける(周期運動)
	//
	// 値は sin。振幅 0(または上下限が同じ値)のチャンネルは何も起きないが、
	// 明示できたほうが読みやすいので channels は分けて持つ
	//--------------------------------------------------------------------------------------
	struct UIProceduralAnimation
	{
		float currentTime = 0.0f;					// 経過時間
		EAnimChannel channels = EAnimChannel::NONE;	// 動かすチャンネル

		Oscillation<Math::Vector2> position = {};	// 位置へ加算(px)
		Oscillation<Math::Vector2> scale = {};		// 大きさへ乗算(1が等倍)
		Oscillation<float> rotation = {};			// 回転へ加算(度)
		Oscillation<Math::Color> color = {};		// 色へ乗算(1が元の色)
	};

	//======================================================================================
	// 方向 : 枠をどの辺に出すか
	//
	// ビットフラグなので値は必ず2の冪にすること。
	// (連番にすると LEFT|RIGHT が UP|DOWN|LEFT と同じ値になってしまう)
	//======================================================================================
	enum class EDirection : uint32_t
	{
		NONE	= 0,
		UP		= 1 << 0,
		DOWN	= 1 << 1,
		LEFT	= 1 << 2,
		RIGHT	= 1 << 3,

		ALL		= UP | DOWN | LEFT | RIGHT,
	};
	ENUM_ATTR_BITFLAG(EDirection);

	// 文字列の行揃え : ブロック全体の位置は pivot が決めるので、ここは行同士の揃え方
	enum class ETextAlign : uint32_t
	{
		Left,
		Center,
		Right,
	};

	//======================================================================================
	// デコレーションの種類
	//======================================================================================
	enum class EDecorationType : uint32_t
	{
		Polygon,	// 板ポリ : 組み込みの白テクスチャを色で染めて出す。テクスチャを用意しなくてよい
		Image,		// 画像 : 指定したテクスチャを出す
		Text,		// 文字 : フォントから文字列を組んで出す
	};

	//======================================================================================
	// デコレーション1つぶん
	//
	// 使わない種類のフィールドはそのまま眠っているだけ。
	// 種類ごとに配列を分けると描画順が種類順に固定されてしまい、
	// 「枠の上に文字、その上にアイコン」のような重ね方ができなくなるため、
	// 1つの struct にまとめて配列1本で持つ
	//======================================================================================
	struct Decoration
	{
		EDecorationType type = EDecorationType::Polygon;

		std::string name = "Decoration";	// エディターの見出し用
		bool isVisible = true;				// 出すか

		/// <summary>
		/// 群番号
		/// </summary>
		/// <remarks>
		/// 「この飾りだけを別の場所へ出したい」HUD 用の仕分け札。
		/// 例) TargetBoxHUD は 0 を通常枠、1 をロック枠として使い分ける。
		/// 使わない UI は全部 0 のままでよい(UIBase::Draw は群を絞らない)
		/// </remarks>
		uint32_t group = 0;

		//----------------------------------------------------------------------------------
		// 親(UI)からの相対トランスフォーム
		//----------------------------------------------------------------------------------
		Math::Vector2 offsetPos = {};			// 親のピボット位置からのずれ(px, 親の回転前)
		Math::Vector2 pixelSize = { 64.0f, 64.0f };	// 大きさ(px) ※Textは fontPixelSize が優先
		float rotation = 0.0f;					// 親の回転へ加算(度)
		float scale = 1.0f;						// 親の倍率へ乗算
		Math::Vector2 pivot = { 0.5f, 0.5f };	// 回転軸/基準点(正規化[0,1], 0.5=中心)
		float layerOffset = 0.0f;				// 親のZへ加算
		Math::Color color = Engine::Color::WHITE;	// 親の色へ乗算

		//----------------------------------------------------------------------------------
		// Polygon / Image 共通
		//----------------------------------------------------------------------------------
		Math::Vector2 uvOffset = {};			// UVスクロール・コマ送り
		Math::Vector2 uvScale = { 1.0f, 1.0f };	// 1枚に並べた絵から1コマ切り出すときの倍率

		//----------------------------------------------------------------------------------
		// Image
		//----------------------------------------------------------------------------------
		Engine::GUID texGUID = {};
		// 実体への参照は保存しない : 読み込み時に texGUID から引き直す
		Engine::ResourceRef<Engine::Resource::Texture> texRef = {};

		//----------------------------------------------------------------------------------
		// 枠(Polygon / Image どちらでも出せる)
		//----------------------------------------------------------------------------------
		bool isFill = true;								// 中を塗るか(false で枠だけ)
		Math::Color edgeColor = Engine::Color::WHITE;	// 枠の色(親の色へ乗算)
		float edgePixel = 0.0f;							// 枠の太さ(px)。0 で枠なし
		EDirection edgeSide = EDirection::ALL;			// どの辺に出すか

		//----------------------------------------------------------------------------------
		// Text
		//----------------------------------------------------------------------------------
		std::string text = "Text";
		Engine::GUID fontGUID = {};
		Engine::ResourceRef<Engine::Resource::Font> fontRef = {};

		float fontPixelSize = 32.0f;			// 出したい文字の高さ(px)
		float lineSpacing = 1.0f;				// 行送りの倍率(1.0 でフォントの既定)
		float charSpacing = 0.0f;				// 字間へ足す量(px)
		ETextAlign textAlign = ETextAlign::Center;

		//----------------------------------------------------------------------------------
		// アニメーション
		//----------------------------------------------------------------------------------
		std::optional<UIAnimation> opTweenAnim;					// 始点から終点へ動かすなら付与
		std::optional<UIProceduralAnimation> opOscillationAnim;	// 揺らし続けるなら付与
	};

	//======================================================================================
	// 親(UI)のトランスフォーム : デコレーションを合成する土台
	//======================================================================================
	struct ParentTransform
	{
		Math::Vector2 pixelPos = {};				// ピボットのスクリーン座標(px)
		float rotation = 0.0f;						// 回転(度)
		float scale = 1.0f;							// 倍率
		float layer = 0.0f;							// Z順
		Math::Color color = Engine::Color::WHITE;	// 全デコレーションへ掛かる色
	};

	//======================================================================================
	// 描くときの一時的な上書き
	//
	// 「同じ見た目を敵の数だけ別の場所へ出す」「桁ごとにUVをずらす」といった、
	// 保存しないその場かぎりの差し替えをここへまとめる。
	// デコレーション側の値を書き換えて描くと、次のフレームまで汚れが残ってしまうため
	//======================================================================================
	struct DrawOverride
	{
		// 位置の差し替え : 親のピクセル座標を無視してここへ出す
		bool isUsePos = false;
		Math::Vector2 pixelPos = {};

		float scale = 1.0f;							// さらに掛ける倍率
		Math::Color tint = Engine::Color::WHITE;	// さらに掛ける色
		Math::Vector2 uvOffsetAdd = {};				// UVオフセットへ足す

		// UV倍率の差し替え : 立てると飾り側の UVScale を無視してこちらを使う
		//
		// 「1枚に並べた絵から1コマ切り出す」ような、コマの幅を出す側が知っている場合に使う
		// (ScoreHUD が桁数から 1/AtlasCount を出すなど)
		bool isUseUvScale = false;
		Math::Vector2 uvScale = { 1.0f, 1.0f };

		// 群の絞り込み : 立てるとこの群のデコレーションだけ描く
		bool isUseGroup = false;
		uint32_t group = 0;
	};

	//======================================================================================
	// 操作
	//======================================================================================

	/// <summary>
	/// アニメーションの時間を進める : 出していないフレームも進める(UIBase::Update から)
	/// </summary>
	void AdvanceAnimation(Decoration& a_decoration, float a_deltaTime);

	/// <summary>
	/// デコレーションを1つ描く
	/// </summary>
	/// <param name="a_pGraphicsEngine">描画命令の積み先</param>
	/// <param name="a_pResourceManager">テクスチャ・フォントを引くのに使う</param>
	/// <param name="a_decoration">描くもの</param>
	/// <param name="a_parent">親(UI)のトランスフォーム</param>
	/// <param name="a_override">その場かぎりの差し替え</param>
	void DrawDecoration(
		Engine::Graphics::GraphicsEngine* a_pGraphicsEngine,
		Engine::Resource::ResourceManager* a_pResourceManager,
		const Decoration& a_decoration,
		const ParentTransform& a_parent,
		const DrawOverride& a_override = {});

	/// <summary>
	/// 保存されているGUIDから、テクスチャとフォントの参照を引き直す
	/// </summary>
	/// <remarks>読み込み直後とエディターでの差し替え時に呼ぶ。実体の到着は待たない</remarks>
	void RequestResources(Decoration& a_decoration, Engine::Resource::ResourceManager* a_pResourceManager);

	/// <summary>
	/// アーカイブ(1つぶん)
	/// </summary>
	/// <remarks>
	/// 配列の1要素として呼ばれる前提。BeginObject / EndObject は呼び出し側が行う。
	/// フィールドを足すときは必ず末尾へ(バイナリは並び順で読むため)
	/// </remarks>
	void ArchiveDecoration(Engine::Persistence::Archive& a_ar, Decoration& a_decoration);

	/// <summary>
	/// インスペクター(1つぶん)
	/// </summary>
	/// <returns>値が変わったら true</returns>
	bool DrawDecorationInspector(Decoration& a_decoration, Engine::Resource::ResourceManager* a_pResourceManager);
}
