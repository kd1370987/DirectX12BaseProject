#pragma once

struct GroundEffectComponent
{
	Math::Ray downRay;			// 下方向に地面を探すレイ

	float maxDistance = 100;
};