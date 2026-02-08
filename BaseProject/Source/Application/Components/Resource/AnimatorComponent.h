#pragma once

struct AnimatorComponent
{
	uint32_t currentClipID = 0;
	float time = 0.0f;

	uint32_t nextClipID = 0;
	float blendTime = 0.0f;

	ECS::Flg isLoop = 0;
};