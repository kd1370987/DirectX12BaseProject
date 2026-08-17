#pragma once
namespace Engine::Particle
{
	/// <summary>
	/// パーティクルの現在データ
	/// 固定されたデータはアセット側から定数バッファで送る
	/// </summary>
	struct ParticleData
	{
		DirectX::XMFLOAT3 pos;		// 現在のワールド座標
		float life;					// 残り寿命

		DirectX::XMFLOAT3 velocity;	// 現在の移動ベクトル
		float size;					// サイズ
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
	/// 描画時にアセット単位で送る定数バッファ
	/// ※ HLSL 側 ParticleDrawData と並びを合わせること
	/// </summary>
	struct ParticleDrawData
	{
		uint32_t	orientation = 0;	// EParticleOrientation
		float		stretch = 1.0f;		// 進行方向への伸ばし倍率
		float		pad0 = 0.0f;
		float		pad1 = 0.0f;
	};
}