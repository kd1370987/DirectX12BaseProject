#pragma once

namespace Engine::Animation
{
	void Interpolate(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Matrix& a_rDst);

	bool InterpolateTranslations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Vector3& a_resullt);
	bool InterpolateRotations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Quaternion& a_resullt);
	bool InterpolateScale(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Vector3& a_resullt);

	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx = -1,
		const Engine::Resource::Model* a_model = nullptr,
		Math::Matrix* a_pOutLocalMat = nullptr,
		Math::Matrix* a_pOutWorldMat = nullptr
	);

	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx,
		const Engine::Resource::Model* a_model,
		std::span <Resource::NodePoseMatrix> a_nodePoseVec
	);
}