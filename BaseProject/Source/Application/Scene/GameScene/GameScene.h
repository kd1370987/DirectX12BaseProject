#pragma once

#include "Engine/Scene/BaseScene/BaseScene.h"

class GameScene : public Engine::Scene::BaseScene
{
public:

	GameScene() = default;
	~GameScene() = default;

private:

	void SetSceneType() override
	{
		m_sceneType = SceneType::Game;
	}

	void Event() override;
	
	void Init() override;
	void Release() override;

	void RegistryComponent() override;
	void RegistrySystem() override;
	void RegistryEntity() override;
};