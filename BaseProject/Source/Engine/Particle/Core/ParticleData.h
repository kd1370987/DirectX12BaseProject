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
}