#include "Animator.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Animation
{
	void Engine::Animation::Animator::Create(Resource::Handle<Resource::Model> a_modelHandle)
	{
		// モデル取得
		auto* _pModel = Resource::ResourceManager::Instance().Get(a_modelHandle);
		if (!_pModel) return;

		m_modelHandle = a_modelHandle;

		if (_pModel->GetUPAnimationVec().size() <= 0) return;

		// スケルタルポーズ初期化
		UINT _nodeNum = _pModel->GetOriginalNodeVec().size();
		m_nodePoseVec.clear();
		m_nodePoseVec.resize(_nodeNum);

		// ボーン行列初期化
		UINT _boneNodeNum = _pModel->GetBoneNodeVec().size();
		m_nodePoseVec.resize(_boneNodeNum);
		for (size_t _i = 0; _i < _boneNodeNum; ++_i)
		{
			m_nodePoseVec[_i] = _pModel->GetOriginalNodeVec()[_i].worldTransform;
		}

		
	}
	void Animator::Play(UINT a_clipID, float a_speed, bool a_isLoop)
	{}
	void Animator::Play(const std::string & a_clipName, float a_speed, bool a_isLoop)
	{}
	void Animator::Stop()
	{}
	void Animator::Update(float a_dt)
	{}
}