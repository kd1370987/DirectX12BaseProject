#include "TitleScene.h"

#include "Engine/Scene/SceneManager/SceneManager.h"

void TitleScene::Event()
{
	if (GetAsyncKeyState('E'))
	{
		Engine::Scene::SceneManager::Instance().SetNextScene(SceneType::Game, Engine::Scene::SceneChangeType::Replace);
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

