#pragma once
namespace Engine::Animation
{
	struct NodePose
	{
		DirectX::XMFLOAT4X4 local = {};
		DirectX::XMFLOAT4X4 world = {};
	};

	class AnimationMatrixManager
	{
	public:

		// ノードポーズ配列確保
		Storage::Range AllocateNodePoseVec(const Resource::Handle<Resource::Model>& a_modelHandle);
		NodePose* AccessNodePoseVec(const Storage::Range& a_range);
		void DeleteNodeRange(const Storage::Range& a_range);

		// ボーン行列配列確保
		Storage::Range AllocateBoneMatVec(const Resource::Handle<Resource::Model>& a_modelHandle);
		DirectX::XMFLOAT4X4* AccessBoneMatVec(const Storage::Range& a_range);
		void DeleteBoneRange(const Storage::Range& a_range);

	private:

		FreeRange m_nodeFreeRangeStorage;								// レンジ管理
	
		std::vector<DirectX::XMFLOAT4X4> m_boneMatStorage;

		FreeRange m_boneFreeRangeStorage;								// レンジ管理
		std::vector<NodePose> m_nodePoseMatStorage;

	private:

		AnimationMatrixManager()
		{
			// とりあえず
			m_nodeFreeRangeStorage.Init(10000);
			m_boneMatStorage.resize(10000);

			m_boneFreeRangeStorage.Init(10000);
			m_nodePoseMatStorage.resize(10000);
		}
		~AnimationMatrixManager() = default;
	public:
		static AnimationMatrixManager& Instance()
		{
			static AnimationMatrixManager _instance;
			return _instance;
		}

	};
}