#include "BaseScene.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void BaseScene::Enter()
{
	SetSceneType();

	Init();

	RegistryComponent();
	RegistrySystem();
	RegistryEntity();

	World::Instance().RunSystem(SystemType::Init,0.0f);
}

void BaseScene::Exit()
{
	Release();
}

void BaseScene::Update(float a_dt)
{
	World::Instance().RunSystem(SystemType::Input, a_dt);

	World::Instance().RunSystem(SystemType::PreUpdate, a_dt);

	World::Instance().RunSystem(SystemType::Update, a_dt);

	World::Instance().RunSystem(SystemType::Physics, a_dt);

	World::Instance().RunSystem(SystemType::Camera, a_dt);

	World::Instance().RunSystem(SystemType::PostUpdate, a_dt);
}

void BaseScene::Draw()
{
	RenderContext::Instance().BeginSimpleRender();

	World::Instance().RunSystem(SystemType::PreDraw, 0.0f);

	World::Instance().RunSystem(SystemType::Draw, 0.0f);
}
