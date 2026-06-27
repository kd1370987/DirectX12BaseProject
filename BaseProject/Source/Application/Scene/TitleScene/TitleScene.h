#pragma once

#include "Engine/Scene/BaseScene/BaseScene.h"

class TitleScene : public Engine::Scene::BaseScene
{
public:

	TitleScene() = default;
	~TitleScene() = default;

private:

	void SetSceneType() override
	{
		m_sceneType = SceneType::Title;
	}

	void Event() override;

	void RegistryComponent() override;
	void RegistrySystem() override;
	void RegistryEntity() override;
};