#pragma once

#include "ModelResource/Material/Material.h"
#include "ModelResource/Animation/Animation.h"
#include "ModelResource/Mesh/Mesh.h"
#include "ModelResource/Node/Node.h"

struct Model
{
	// マテリアル
	std::vector<Material>						materials;				// マテリアルの配列

	// アニメーション
	std::vector<std::shared_ptr<AnimationData>> spAnimations;				// データリスト

	// ノード
	std::vector<Node>							originalNodes;				// 全ノード配列
	std::vector<int>							rootNodeIndices;			// Rootノード
	std::vector<int>							boneNodeIndices;			// ボーンノード
	std::vector<int>							meshNodeIndices;			// メッシュが存在するノード
	std::vector<int>							collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
	std::vector<int>							drawMeshNodeIndices;		// 描画するノード
};

namespace Animation
{
	void Interpolate(AnimationNode& a_node, float a_currentTime, DirectX::XMFLOAT4X4& a_rDst);

	bool InterpolateTranslations(AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);
	bool InterpolateRotations(AnimationNode& a_node, float a_currentTime, DXSM::Quaternion& a_resullt);
	bool InterpolateScale(AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);

	/// <summary>
	/// 二分探索で、指定時間から次の配列要素のkeyIndexを求める
	/// </summary>
	/// <param name="a_list">キー配列</param>
	/// <returns>次の配列要素のインデックス</returns>
	template<class T>
	int BinarySearchNextAnimKey(const std::vector<T>& a_list, float a_currentTime)
	{
		int _low = 0;
		int _high = static_cast<int>(a_list.size());

		while (_low < _high)
		{
			int _mid = (_low + _high) / 2;
			float _midTime = a_list[_mid].time;

			if (_midTime <= a_currentTime)
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


	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx = -1,
		Model* a_model = nullptr,
		DirectX::XMFLOAT4X4* a_pOutLocalMat = nullptr,
		DirectX::XMFLOAT4X4* a_pOutWorldMat = nullptr
	);
}