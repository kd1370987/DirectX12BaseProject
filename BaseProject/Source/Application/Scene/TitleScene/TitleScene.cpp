#include "TitleScene.h"

#include "../SceneManager.h"

void TitleScene::Event()
{
	if (GetAsyncKeyState('E'))
	{
		SceneManager::Instance().SetNextScene(SceneType::Game,SceneChangeType::Replace);
	}
}

void TitleScene::RegistryComponent()
{
}

void TitleScene::RegistrySystem()
{
}

void TitleScene::RegistryEntity()
{
}

