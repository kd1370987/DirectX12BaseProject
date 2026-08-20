#pragma once
namespace Engine::Particle
{
	/// <summary>
	/// CPUが毎フレーム計算して Uploadヒープ経由で
	/// いまフレーム、どこから何個出すかの命令
	/// </summary>
	/// <remarks>
	/// HLSL 側 EmitData(Common/RootParameters/Particle.hlsli)と並びを合わせること
	/// </remarks>
	struct EmitterData
	{
		DirectX::XMFLOAT3 emitPos;		// 発生源のワールド座標
		UINT emitCount;					// 発生させる数

		DirectX::XMFLOAT3 emitDirection;	// 発生させたい方向
		float baseScale;					// エミッター専用のスケール

		// ---- ランダム要素 ----
		float positionRadius;		// 発生位置の半径
		float directionAngle;		// 方向のばらつき角度 (ラジアン。Cone のときだけ使う)

		// 拡縮区間
		float minScale;
		float maxScale;

		// スピード区間
		float minSpeed;
		float maxSpeed;

		// 生存時間区間
		float minLifeTime;
		float maxLifeTime;

		// 発生方向の決め方(EParticleEmitShape)。
		// Cone のときだけ directionAngle が効く
		UINT emitShape;

		// 出した粒に持たせる発生源の番号。
		// ローカル空間で回すときだけ 1 以上になる(0 は単位行列 = ワールド空間)
		UINT emitterIndex;

		float pad0;
		float pad1;
	};
}