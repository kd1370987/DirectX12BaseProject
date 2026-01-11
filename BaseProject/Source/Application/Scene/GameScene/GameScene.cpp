#include "GameScene.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"

#include "Engine/GPUResource/Model/Model.h"

// ECS関連
#include "Engine/ECS/World/World.h"

// コンポーネント関連
#include "../../Components/Transform/TRSComponent.h"
#include "../../Components/Transform/WorldMatrixComponent.h"

#include "../../Components/Resource/ModelComponent.h"

// システム関連
#include "../../Systems/DrawSystem.h"
#include "../../Systems/Update/CalcMatrix/CalcMatrixSystem.h"


void GameScene::Enter()
{
	BaseScene::Enter();

	// カメラ座標設定
	auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f);
	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	// コンポーネント登録
	World::Instance().RegisterComponentType<TRSComponent>("Transform");
	World::Instance().RegisterComponentType<WorldMatrixComponent>("WorldMatrix");
	World::Instance().RegisterComponentType<ModelComponent>("Model");

	// システム登録
	World::Instance().RegisterSystem<DrawSystem>();
	World::Instance().RegisterSystem<CalcMatrixSystem>();

	// エンティティ生成
	for(size_t _x = 0; _x < 10; ++_x)
	{
		for (size_t _y = 0; _y < 10; ++_y)
		{
			ECS::Signature _sig;
			_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
			_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
			_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
			ECS::Entity _entity = World::Instance().CreateEntity(_sig);

			ModelComponent* _model = World::Instance().RefData<ModelComponent>(_entity);
			_model->modelID = ResourceManager::Instance().GetModel("Asset/Model/tank/tank.gltf");
			_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
			_model->emissiveScale = { 0.0f,0.0f,0.0f };
			TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
			_ref->pos = { -50.f + (_x * 5), -15.0f, 10.0f + (_y * 5) };
			_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
			_ref->scale = { 1.0f,1.0f,1.0f };
		}
	}

	ECS::Signature _sig;
	_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
	_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
	_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
	auto _entity = World::Instance().CreateEntity(_sig);

	ModelComponent* _model = World::Instance().RefData<ModelComponent>(_entity);
	_model->modelID = ResourceManager::Instance().GetModel("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX");
	_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
	_model->emissiveScale = { 0.0f,0.0f,0.0f };
	TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
	_ref->pos = { 0.0f, -100.0f, 200.0f };
	_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
	_ref->scale = { 1.0f,1.0f,1.0f };
}

void GameScene::Exit()
{
	BaseScene::Exit();
}

void GameScene::Update()
{
	World::Instance().RunSystem(SystemType::Update, 0.0f);
}

void GameScene::Draw()
{
	RenderContext::Instance().BeginSimpleRender();

	RenderContext::Instance().SetToShader(m_cameraMat);

	World::Instance().RunSystem(SystemType::Draw,0.0f);

}
