#pragma once

struct AnimatorComponent
{
	uint32_t clipID = 0;
	float time = 0.0f;
	float speed = 1.0f;

	Engine::ECS::Flg isLoop = 0;
};