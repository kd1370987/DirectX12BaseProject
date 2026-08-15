#pragma once

#include "Engine/Graphics/CBData.h"
#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	// ブルームのオプションを、そのまま定数バッファへ送れる形へ詰め替える。
	//
	// 抽出パスと合成パスが同じ CBBloomOption(b13) を見るので、
	// 詰め方が2箇所でズレないようここへ寄せておく。
	inline BloomOptionCB MakeBloomOptionCB()
	{
		const auto& _bloomOp = Option::OptionManager::GetInstance().GetBloomOption();

		BloomOptionCB _cb = {};
		_cb.threshold = _bloomOp.threshold;
		_cb.softKnee  = _bloomOp.softKnee;
		_cb.intensity = _bloomOp.intensity;
		_cb.enable    = _bloomOp.enable ? 1 : 0;

		return _cb;
	}
}
