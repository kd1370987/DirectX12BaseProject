#pragma once
namespace Engine::Particle
{
	/// <summary>
	/// CPUが毎フレーム計算して Uploadヒープ経由で
	/// いまフレーム、どこから何個出すかの命令
	/// </summary>
	struct EmitterData
	{
		DirectX::XMFLOAT3 emitPos;		// 発生源のワールド座標
		UINT emitCount;					// 発生させる数

		DirectX::XMFLOAT3 emitDirection;	// 発生させたい方向
		float baseScale;					// エミッター専用のスケール

		// ---- ランダム要素 ----
		float positionRadius;		// 発生位置の半径
		float directionAngle;		// 方向のばらつき角度 (度)

		// 拡縮区間
		float minScale;
		float maxScale;

		// スピード区間
		float minSpeed;
		float maxSpeed;

		// 生存時間区間
		float minLifeTime;
		float maxLifeTime;
	};
}