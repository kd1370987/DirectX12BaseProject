#pragma once

//==========================================================================================
// WarmGroundEffectComponent
// 
// 砂漠で潜った際に地表にエフェクトを炊く際のコンポーネント
//==========================================================================================
struct BoidLeaderComponent
{
	Math::Ray upRay;			// 上方向にレイを打って地面があるかチェック
	float maxDistance = 100;	// 最大射程

	Engine::Handle<Engine::Resource::EffectAsset> effectHandle;
};
