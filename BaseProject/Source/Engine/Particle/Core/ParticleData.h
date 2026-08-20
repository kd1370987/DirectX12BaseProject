#pragma once
namespace Engine::Particle
{
	/// <summary>
	/// パーティクルの現在データ
	/// 固定されたデータはアセット側から定数バッファで送る
	/// </summary>
	/// <remarks>
	/// HLSL 側 ParticleData(Common/RootParameters/Particle.hlsli)と並びを合わせること。
	/// GPU のプールはこの sizeof で確保されるので、増やせばそのぶん VRAM を食う
	/// (容量 10000 なら 16 バイト増やして 160KB)。
	/// </remarks>
	struct ParticleData
	{
		DirectX::XMFLOAT3 pos;		// 現在のワールド座標
		float life;					// 残り寿命

		DirectX::XMFLOAT3 velocity;	// 現在の移動ベクトル
		float size;					// サイズ

		// 発生した時点の寿命。
		// 寿命は粒ごとにランダムなので、「今どこまで進んだか(0〜1)」を出すには
		// 割る相手を粒自身が覚えているしかない。サイズ・色・フェードがこれを使う
		float startLife;
		DirectX::XMFLOAT3 pad;
	};

	/// <summary>
	/// 板ポリの向きの決め方
	/// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
	/// ※ HLSL 側 PARTICLE_ORIENT_*(Common/RootParameters/Particle.hlsli)と数値を合わせること
	/// </summary>
	enum class EParticleOrientation : uint32_t
	{
		Billboard,			// 常にカメラ正面(従来)
		VelocityBillboard,	// カメラ正面のまま、画面上で進行方向へ回す(火花・破片向き)
		VelocityAxis,		// 進行方向をワールドの縦軸にする(手前へ向かうと縮む。弾道向き)
	};

	/// <summary>
	/// 発生方向の決め方
	/// </summary>
	/// <remarks>
	/// Cone の角度を 360 にしても全方向にはならない(円錐の半頂角なので、
	/// 全方向にしたければ 180 度で、しかもその極限は分布が偏る)。
	/// 爆発のように四方八方へ飛ばしたいときは Sphere を選ぶこと。
	///
	/// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
	/// ※ HLSL 側 PARTICLE_EMIT_SHAPE_* と数値を合わせること
	/// </remarks>
	enum class EParticleEmitShape : uint32_t
	{
		Cone,			// 指定方向を軸にした円錐(噴射・排気。従来の挙動)
		Sphere,			// 中心から全方向へ均等(爆発)
		Hemisphere,		// 指定方向側の半球だけ(地面での爆発など)
	};

	/// <summary>
	/// 色の重ね方
	/// </summary>
	/// <remarks>
	/// Additive は「光」を足す。重ねるほど明るくなり、描く順番で結果が変わらないので
	/// 並べ替えが要らない。火花・炎・爆発の芯はこちら。
	///
	/// AlphaBlend は「物」を前に置く。煙や破片のように光っていないものを
	/// 加算で出すと背景を明るくしてしまうので、そちらはこちらを使う。
	/// ただし奥から順に描かないと正しく重ならない。パーティクルは並べ替えていないので、
	/// 同じアセットの粒同士が重なるところで前後が入れ替わって見えることがある。
	///
	/// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
	/// </remarks>
	enum class EParticleBlendMode : uint32_t
	{
		Additive,		// 加算合成(従来)
		AlphaBlend,		// 半透明合成
	};

	/// <summary>
	/// 描画時にアセット単位で送る定数バッファ
	/// ※ HLSL 側 ParticleDrawData と並びを合わせること
	/// ※ 定数バッファなので float4 単位で行が変わる。並べ替えるときは両側を必ず揃えること
	/// </summary>
	struct ParticleDrawData
	{
		uint32_t	orientation = 0;		// EParticleOrientation
		float		stretch = 1.0f;			// 進行方向への伸ばし倍率
		float		endSizeScale = 1.0f;	// 寿命の終わりでのサイズ倍率(1で変化なし)
		float		fadeInRatio = 0.0f;		// 頭から不透明になるまでの割合(0でフェードインなし)

		float		fadeOutRatio = 0.0f;	// 末尾で透明になるまでの割合(0でフェードアウトなし)
		float		pad0 = 0.0f;
		float		pad1 = 0.0f;
		float		pad2 = 0.0f;

		Math::Color	startColor = { 1.0f, 1.0f, 1.0f, 1.0f };	// 発生時の色
		Math::Color	endColor = { 1.0f, 1.0f, 1.0f, 1.0f };		// 消える直前の色
	};
}
