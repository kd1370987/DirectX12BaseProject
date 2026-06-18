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
	};
}