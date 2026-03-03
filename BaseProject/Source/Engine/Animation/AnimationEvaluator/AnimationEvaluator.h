#pragma once

namespace Animation
{
	void Interpolate(Engine::Resource::AnimationNode& a_node, float a_currentTime, DirectX::XMFLOAT4X4& a_rDst);

	bool InterpolateTranslations(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);
	bool InterpolateRotations(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Quaternion& a_resullt);
	bool InterpolateScale(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt);

	void CalcNodeMatrix(
		int a_nodeIdx,
		int a_parentNodeIdx = -1,
		Engine::Resource::Model* a_model = nullptr,
		DirectX::XMFLOAT4X4* a_pOutLocalMat = nullptr,
		DirectX::XMFLOAT4X4* a_pOutWorldMat = nullptr
	);
}