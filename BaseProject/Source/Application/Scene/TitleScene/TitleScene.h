#pragma once

#include "../BaseScene/BaseScene.h"

class TitleScene : public BaseScene
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