#pragma once
namespace App::Object::Decoration
{
	template<typename T>
	struct AnimElement
	{
		T start = {};
		T end = {};
	};

	template<typename T>
	struct Oscillation
	{
		T amplitude = {};
		float frequency = 1.0f;
		float phase = 0.0f;
	};

	// 動きを持たせるため
	struct UIAnimation
	{
		float currentTime = 0.0f;		// 経過時間
		float durationTime = 0.0f;		// 変化するのにかかる時間
		bool isLoop = false;				// ループ設定

		// TweenAnimデータ
		AnimElement<Math::Color> color = {};
		AnimElement<Math::Vector2> position = {};
		AnimElement<Math::Vector2> scale = {};
		AnimElement<float> rotation = {};
		AnimElement<Math::Vector2> uv = {};
	};

	struct UIProceduralAnimation
	{
		// 周期運動
		Oscillation<Math::Vector2> position;
		Oscillation<Math::Vector2> scale;
		Oscillation<float> rotation;
	};

	// 方向
	enum class EDirection
	{
		NONE,
		UP,
		DOWN,
		LEFT,
		RIGHT,
		ALL
	};
	ENUM_ATTR_BITFLAG(EDirection);

	// いたポリのデコレーション
	struct DecorationPolygon
	{
		// ---- 本体 ----
		Math::Color color = Engine::Color::WHITE;		// メインカラー

		// 座標系
		Math::Vector2 pixelPos = {};
		Math::Vector2 pixelSize = {};			// ピクセルサイズ
		float rotation = 0.0f;

		// オプション
		Math::Vector2 pivot = { 0.5f, 0.5f };	// 回転軸/基準点(正規化[0,1], 0.5=中心)
		Math::Vector2 uvOffset = {};			// UVスクロールなど
		float layer = 0.0f;						// Z位置
		Math::Vector2 editSize = {};			// エディターでいじる際のピクセルサイズ
		float scale = 1.0f;						// 等倍スケール用

		// ---- 辺 ----
		Math::Color edgeColor = {};				// エッジの色
		uint32_t edgePixel = 0;					// エッジの太さ
		EDirection viewSide = EDirection::ALL;	// エッジの出現方向

		// ---- UI アニメーション ----
		std::optional<UIAnimation> opTweenAnim;						// アニメーションするのなら付与
		std::optional<UIProceduralAnimation> opOscillationAnim;		// アニメーションするのなら付与
	}; 

	// テクスチャのデコレーション
	struct DecorationImage
	{
		// 描画するUIの構成テクスチャ
		Engine::ResourceRef<Engine::Resource::Texture> m_texRef = {};
		Engine::GUID m_texGUID = {};

		// 色
		Math::Color m_color = Engine::Color::WHITE;

		// 座標系
		Math::Vector2 m_pixelPos = {};
		Math::Vector2 m_pixelSize = {};			// ピクセルサイズ
		float m_rotation = 0.0f;

		// オプション
		Math::Vector2 m_pivot = { 0.5f, 0.5f };	// 回転軸/基準点(正規化[0,1], 0.5=中心)
		Math::Vector2 m_uvOffset = {};			// UVスクロールなど
		float m_layer = 0.0f;					// Z位置
		Math::Vector2 m_editSize = {};			// エディターでいじる際のピクセルサイズ
		float m_scale = 1.0f;					// 等倍スケール用

		// ---- UI アニメーション ----
		std::optional<UIAnimation> opTweenAnim;						// アニメーションするのなら付与
		std::optional<UIProceduralAnimation> opOscillationAnim;		// アニメーションするのなら付与
	};

	// フォント描画
	struct DecorationText
	{
		std::string text = "";		// 描画させたい文字列
		float scale = 0.0f;
		Math::Color color = {};

		// ---- UI アニメーション ----
		std::optional<UIAnimation> opTweenAnim;						// アニメーションするのなら付与
		std::optional<UIProceduralAnimation> opOscillationAnim;		// アニメーションするのなら付与
	};
}