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
		UINT _maxAniNodeNum = 0;
		for (auto& _anim : _pModel->GetUPAnimationVec())
		{
			UINT _size = _anim->nodes.size();
			_maxAniNodeNum = std::max(_maxAniNodeNum,_size);
		}
		m_nodePoseVec.resize(_maxAniNodeNum);

		// ボーン行列初期化
		UINT _boneNodeNum = _pModel->GetBoneNodeVec().size();
		m_boneVec.resize(_boneNodeNum);
		for (size_t _i = 0; _i < _boneNodeNum; ++_i)
		{
			m_boneVec[_i] = _pModel->GetOriginalNodeVec()[_i].worldTransform;
		}

		
	}
	void Animator::Play(UINT a_clipID, float a_speed, bool a_isLoop)
	{
		// モデル取得
		auto* _pModel = Resource::ResourceManager::Instance().Get(m_modelHandle);
		if (!_pModel) return;

		// アニメーションを取得
		auto* _pAniData = _pModel->GetAnimation(a_clipID);
		if (!_pAniData) return;

		m_clipID = a_clipID;
		m_speed = a_speed;
		m_isLoop = a_isLoop;
	}

	void Animator::Play(const std::string & a_clipName, float a_speed, bool a_isLoop)
	{
		// モデル取得
		auto* _pModel = Resource::ResourceManager::Instance().Get(m_modelHandle);
		if (!_pModel) return;

		// アニメーションを取得
		UINT _id = _pModel->GetAnimationClipCount(a_clipName);
		auto* _pAniData = _pModel->GetAnimation(_id);
		if (!_pAniData) return;

		m_clipID = _id;
		m_speed = a_speed;
		m_isLoop = a_isLoop;
	}
	void Animator::Stop()
	{
		m_speed = 0.0f;
	}
	void Animator::SetSpeed(float a_speed)
	{
		m_speed = a_speed;
	}
	void Animator::Update(float a_dt)
	{
		// モデル取得
		auto* _pModel = Resource::ResourceManager::Instance().Get(m_modelHandle);
		if (!_pModel) return;

		// アニメーションを取得
		auto* _pAniData = _pModel->GetAnimation(m_clipID);
		if (!_pAniData) return;

		// すべてのアニメーションノードの行列補間を実行する
		for (auto& _aniNode : _pAniData->nodes)
		{
			UINT _idx = _aniNode.nodeOffset;
			Engine::Animation::Interpolate(_pAniData->nodes[_idx], m_time, m_nodePoseVec[_idx]);
		}

		// アニメーションタイム進行
		m_time += a_dt * m_speed;
		if (m_time >= _pAniData->maxLength)
		{
			if (m_isLoop)
			{
				m_time = 0;
			}
			else
			{
				m_time = _pAniData->maxLength;
			}
		}
	}
}