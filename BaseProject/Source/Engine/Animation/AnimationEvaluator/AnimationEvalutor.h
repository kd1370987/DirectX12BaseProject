#pragma once

namespace Engine::Animation
{
	struct NodePose;

	void Interpolate(const Engine::Resource::AnimationNode& a_node, float a_currentTime, DirectX::XMFLOAT4X4& a_rDst);

	bool InterpolateTranslations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);
	bool InterpolateRotations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Quaternion& a_resullt);
	bool InterpolateScale(const Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);

	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx = -1,
		const Engine::Resource::Model* a_model = nullptr,
		DirectX::XMFLOAT4X4* a_pOutLocalMat = nullptr,
		DirectX::XMFLOAT4X4* a_pOutWorldMat = nullptr
	);
	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx,
		const Engine::Resource::Model* a_model,
		NodePose* a_pNodePoseVec
	);
}