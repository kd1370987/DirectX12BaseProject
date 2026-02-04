#pragma once

#include "../BaseScene/BaseScene.h"

class GameScene : public BaseScene
{
public:

	GameScene() = default;
	~GameScene() = default;

private:

	void SetSceneType() override
	{
		m_sceneType = SceneType::Game;
	}

	void Init() override;

	void RegistryComponent() override;
	void RegistrySystem() override;
	void RegistryEntity() override;
};