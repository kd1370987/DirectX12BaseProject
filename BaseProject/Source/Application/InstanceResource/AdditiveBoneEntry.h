#pragma once

#include "Engine/Resource/Data/AnimatorAsset/AdditivePoseTypes.h"

//==========================================================================================
//
// 加算対象ボーン1本分の実行時データ。
//
// マスターは AnimatorAsset が持つ AdditiveBoneDef で、
// AdditivePoseLinkSystem が「ノード名ハッシュ → nodeIdx」を解決してここへ展開する。
// つまりこれは解決済みキャッシュであり、保存対象ではない。
//
//==========================================================================================
struct AdditiveBoneEntry
{
	int									nodeIdx = -1;						// 対象ノード(解決済み)
	float								share = 1.0f;						// このボーンが受け持つ配分
	DirectX::XMFLOAT3					axisScale = { 1.0f, 1.0f, 1.0f };	// Lag用: 各軸の効き(符号で左右反転)
	Engine::Resource::EAdditiveChannel	channel = Engine::Resource::EAdditiveChannel::Aim;
};
