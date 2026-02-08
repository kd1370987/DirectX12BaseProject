#pragma once

//==========================================================
// アニメーションキー（クォータニオン : 回転など）
//==========================================================
struct AnimationKeyQuaternion
{
	float				time = 0;		// 時間
	DirectX::XMFLOAT4	quat = {};			// クォータニオンデータ
};

//==========================================================
// アニメーションキー（ベクトル : 座標,拡縮など）
//==========================================================
struct AnimationKeyXMFLOAT3
{
	float				time = 0;		// 時間
	DirectX::XMFLOAT3	vec = {};			// 3Dベクトルデータ
};

//==========================================================
// アニメーションノード
//==========================================================
struct AnimationNode
{
	int									nodeOffset = -1;	// 対象ノードのオフセット

	// 各チャンネル
	std::vector<AnimationKeyXMFLOAT3>	translations = {};	// 座標キーリスト
	std::vector<AnimationKeyQuaternion> rotations = {};		// 回転キーリスト
	std::vector<AnimationKeyXMFLOAT3>	scales = {};		// 拡縮キーリスト
};

//==========================================================
// アニメーションデータ
//==========================================================
struct AnimationData
{
	std::string						name = "none";				// アニメーション名
	float							maxLength = 0.0f;		// アニメーションの最大長さ(単位:フレーム)
	std::vector<AnimationNode>		nodes = {};				// アニメーションノードリスト
};

namespace Animation
{
	void Interpolate(AnimationNode& a_node,DirectX::XMFLOAT4X4& a_rDst);

	bool InterpolateTranslations(AnimationNode& a_node,DXSM::Vector3& a_resullt);
	bool InterpolateRotations(AnimationNode& a_node, DXSM::Quaternion& a_resullt);
	bool InterpolateScale(AnimationNode& a_node, DXSM::Vector3& a_resullt);

	/// <summary>
	/// 二分探索で、指定時間から次の配列要素のkeyIndexを求める
	/// </summary>
	/// <param name="a_list">キー配列</param>
	/// <returns>次の配列要素のインデックス</returns>
	template<class T>
	int BinarySearchNextAnimKey(const std::vector<T>& a_list)
	{
		int _low = 0;
		int _high = static_cast<int>(a_list.size());

		while (_low < _high)
		{
			int _mid = (_low + _high) / 2;
			float _midTime = a_list[_mid].time;

			if (_midTime <= 1.0f)
			{
				_low = _low + 1;
			}
			else
			{
				_high = _mid;
			}
		}
		return _low;
	}
}