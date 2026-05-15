#include "AnimationMatrixManager.h"


#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Animation
{
	Storage::Range Engine::Animation::AnimationMatrixManager::AllocateNodePoseVec(const Resource::Handle<Resource::Model>& a_modelHandle)
	{
		// モデルからアニメーションのノードを取得
		const auto* _pModel = Resource::ResourceManager::Instance().Get(a_modelHandle);
		if (!_pModel) return Storage::Range();

		// モデルのアニメーションから最大ノードを持つものを取得
		UINT _maxAnimNodeNum = 0;
		for (auto& _upAnim : _pModel->GetUPAnimationVec())
		{
			UINT _size = _upAnim->nodes.size();
			_maxAnimNodeNum = std::max(_maxAnimNodeNum, _size);
		}
		//static_cast<uint16_t>();

		// レンジ確保
		return m_nodeFreeRangeStorage.Allocate(_pModel->GetOriginalNodeVec().size());
	}
	NodePose* AnimationMatrixManager::AccessNodePoseVec(const Storage::Range& a_range)
	{
		return &m_nodePoseMatStorage[a_range.startIndex];
	}
	void AnimationMatrixManager::DeleteNodeRange(const Storage::Range& a_range)
	{
		m_nodeFreeRangeStorage.Free(a_range);
	}


	Storage::Range AnimationMatrixManager::AllocateBoneMatVec(const Resource::Handle<Resource::Model>& a_modelHandle)
	{
		// モデルからアニメーションのノードを取得
		const auto* _pModel = Resource::ResourceManager::Instance().Get(a_modelHandle);
		if (!_pModel) return Storage::Range();

		// ボーン行列取得
		UINT _boneNodeNum = _pModel->GetBoneNodeVec().size();
		
		return m_boneFreeRangeStorage.Allocate(_boneNodeNum);
	}
	DirectX::XMFLOAT4X4* AnimationMatrixManager::AccessBoneMatVec(const Storage::Range& a_range)
	{
		return &m_boneMatStorage[a_range.startIndex];
	}
	void AnimationMatrixManager::DeleteBoneRange(const Storage::Range& a_range)
	{
		m_boneFreeRangeStorage.Free(a_range);
	}
}