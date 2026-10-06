#pragma once
namespace Engine::Animation
{
	/// アニメーションノード1つぶんのキーを補間して、ローカル行列へ書く。
	/// チャンネルが1つも無ければ a_rDst は触らない
	void Interpolate(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Matrix& a_rDst);

	/// 2つのローカル行列を TRS に分けて補間する(S / T は Lerp、R は Slerp)。
	/// a_weight が 0 なら a_base、1 なら a_layer をそのまま返す
	Math::Matrix BlendLocalMatrix(const Math::Matrix& a_base, const Math::Matrix& a_layer, float a_weight);

	namespace Internal
	{
		bool InterpolateTranslations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Vector3& a_resullt);
		bool InterpolateRotations(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Quaternion& a_resullt);
		bool InterpolateScale(const Engine::Resource::AnimationNode& a_node, float a_currentTime, Math::Vector3& a_resullt);
	}

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
