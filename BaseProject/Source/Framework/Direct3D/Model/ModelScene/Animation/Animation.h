#pragma once

//==========================================================
// アニメーションキー（クォータニオン : 回転など）
//==========================================================
struct AnimationKeyQuaternion
{
	float				time = 0;		// 時間
	DirectX::XMFLOAT4	quat;			// クォータニオンデータ
};

//==========================================================
// アニメーションキー（ベクトル : 座標,拡縮など）
//==========================================================
struct AnimationKeyXMFLOAT3
{
	float				time = 0;		// 時間
	DirectX::XMFLOAT3	vec;			// 3Dベクトルデータ
};

//==========================================================
// アニメーションノード
//==========================================================
struct AnimationNode
{
	int									nodeOffset = -1;	// 対象ノードのオフセット

	// 各チャンネル
	std::vector<AnimationKeyXMFLOAT3>	translations;		// 座標キーリスト
	std::vector<AnimationKeyQuaternion> rotations;			// 回転キーリスト
	std::vector<AnimationKeyXMFLOAT3>	scales;				// 拡縮キーリスト
};