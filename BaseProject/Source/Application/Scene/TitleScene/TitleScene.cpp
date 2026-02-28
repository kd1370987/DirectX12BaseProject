#include "TitleScene.h"

#include "../SceneManager.h"

void TitleScene::Event()
{
	if (GetAsyncKeyState('E'))
	{
		SceneManager::Instance().ReplaceScene(SceneType::Game);
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

