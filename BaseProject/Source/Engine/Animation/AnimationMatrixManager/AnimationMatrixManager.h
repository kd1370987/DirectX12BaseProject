#pragma once
namespace Engine::D3D12
{
	class CommandList;
}

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

		// 初期化
		void Init(ID3D12Device* a_pDevice,D3D12::CommandList& a_cmdList,UINT a_maxElement);

		// ノードポーズ配列確保
		Storage::Range AllocateNodePoseVec(const Handle<Resource::Model>& a_modelHandle);
		NodePose* AccessNodePoseVec(const Storage::Range& a_range);
		void DeleteNodeRange(const Storage::Range& a_range);

		// ボーン行列配列確保
		Storage::Range AllocateBoneMatVec(const Handle<Resource::Model>& a_modelHandle);
		DirectX::XMFLOAT4X4* AccessBoneMatVec(const Storage::Range& a_range);
		void DeleteBoneRange(const Storage::Range& a_range);
		const std::vector<DirectX::XMFLOAT4X4>& GetBoneMatStorage() const { return m_boneMatStorage; }

	private:

		FreeRange m_nodeFreeRangeStorage;								// レンジ管理
		std::vector<DirectX::XMFLOAT4X4> m_boneMatStorage;

		FreeRange m_boneFreeRangeStorage;								// レンジ管理
		std::vector<NodePose> m_nodePoseMatStorage;

	private:

		AnimationMatrixManager() = default;
		~AnimationMatrixManager() = default;
	public:
		static AnimationMatrixManager& Instance()
		{
			static AnimationMatrixManager _instance;
			return _instance;
		}

	};
}