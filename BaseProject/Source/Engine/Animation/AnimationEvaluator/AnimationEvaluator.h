#pragma once

struct AnimationNode;

namespace Animation
{
	void Interpolate(AnimationNode& a_node, float a_currentTime, DirectX::XMFLOAT4X4& a_rDst);

	bool InterpolateTranslations(AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);
	bool InterpolateRotations(AnimationNode& a_node, float a_currentTime, DXSM::Quaternion& a_resullt);
	bool InterpolateScale(AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);

	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx = -1,
		Model* a_model = nullptr,
		DirectX::XMFLOAT4X4* a_pOutLocalMat = nullptr,
		DirectX::XMFLOAT4X4* a_pOutWorldMat = nullptr
	);
}