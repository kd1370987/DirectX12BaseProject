#pragma once

#include "../BaseScene/BaseScene.h"

class ModelResource;

class GameScene : public BaseScene
{
public:

	GameScene() = default;
	~GameScene() = default;

	void Enter() override;

	void Exit() override;

	void Update(float a_dt) override;

	void Draw() override;

private:

	void SetSceneType() override
	{
		m_sceneType = SceneType::Game;
	}

};