#include "Model.h"

uint32_t ModelUtility::GetAnimationClipCount(const Model& a_model, const std::string& a_animeNmae)
{
	// アニメーション取得
	for(size_t _i = 0; _i < a_model.spAnimations.size(); ++_i)
	{
		if (a_model.spAnimations[_i]->name == a_animeNmae)
		{
			return static_cast<uint32_t>(_i);
		}
	}

	assert(false && "アニメーションが見つかりませんでした");
	return MAX_ANIMATIONCLIP;
}

std::shared_ptr<AnimationData> ModelUtility::GetSPAnimation(const Model& a_model, uint32_t a_clipID)
{
	// アニメーション取得
	std::shared_ptr<AnimationData> _spAni = nullptr;
	
	if (a_clipID < a_model.spAnimations.size())
	{
		_spAni = a_model.spAnimations[a_clipID];
	}

	return _spAni;
}
