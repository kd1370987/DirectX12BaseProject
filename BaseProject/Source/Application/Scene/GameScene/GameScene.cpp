#include "GameScene.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"

#include "Engine/GPUResource/Model/Model.h"

#include "Engine/ECS/World/World.h"

#include "../../Components/TransformComponent.h"
#include "../../Components/ModelComponent.h"


void GameScene::Enter()
{
	BaseScene::Enter();

	// ECS移行時には特定のクラスがモデルを持つことすらなくなるから、ウィークポインタ関連の挙動は後回し
	for (int _i = 0; _i < 5; ++_i)
	{
		m_wpModel[_i] = ResourceManager::Instance().GetModel("Asset/Model/tank/tank.gltf");
	}

	m_wpModel2 = ResourceManager::Instance().GetModel("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX");

	// カメラ座標設定
	auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f);
	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	m_charaMat = DirectX::XMFLOAT4X4
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	m_charaMat2 = DirectX::XMFLOAT4X4
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		5.0f,-100.0f, 200.0f,1.0f
	};

	ECS::Signature _sig;
	_sig.set(World::Instance().GetCompTypeID(typeid(TransformComponent)));
	_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
	m_entity = World::Instance().CreateEntity(_sig);

	ModelComponent* _model = reinterpret_cast<ModelComponent*>(World::Instance().RefData(m_entity, typeid(ModelComponent)));
	_model->modelID = ResourceManager::Instance().GetModelID("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX");



	TransformComponent* _ref = reinterpret_cast<TransformComponent*>(World::Instance().RefData(m_entity, typeid(TransformComponent)));
	_ref->worldMat = DirectX::XMFLOAT4X4
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		5.0f,-100.0f, 200.0f,1.0f
	};
}

void GameScene::Exit()
{
	BaseScene::Exit();
}

void GameScene::Update()
{
	m_rotateY += 0.05f;

	TransformComponent* _ref = reinterpret_cast<TransformComponent*>(World::Instance().RefData(m_entity, typeid(TransformComponent)));
	

	DirectX::XMMATRIX _rotY2 = DirectX::XMMatrixRotationY(-m_rotateY);
	DirectX::XMMATRIX _trans2 = DirectX::XMMatrixTranslation(50.0f, -100.0f, 200.0f);
	_rotY2 = DirectX::XMMatrixMultiply(_rotY2, _trans2);
	//DirectX::XMStoreFloat4x4(&m_charaMat2, _rotY2);
	DirectX::XMStoreFloat4x4(&_ref->worldMat, _rotY2);

	if (GetAsyncKeyState('A'))
	{
		m_moveX -= 0.1f;
	}
	if (GetAsyncKeyState('D'))
	{
		m_moveX += 0.1f;
	}
}

void GameScene::Draw()
{
	RenderContext::Instance().BeginSimpleRender();

	RenderContext::Instance().SetToShader(m_cameraMat);

	for (int _i = 0; _i < 5; ++_i)
	{

		DirectX::XMMATRIX _rotY = DirectX::XMMatrixRotationY(m_rotateY);
		DirectX::XMMATRIX _trans = DirectX::XMMatrixTranslation(0.0f + m_moveX, -15.0f, 10.0f + (_i * 5));
		_rotY = DirectX::XMMatrixMultiply(_rotY, _trans);
		DirectX::XMStoreFloat4x4(&m_charaMat, _rotY);

		RenderContext::Instance().DrawModel(
			m_wpModel[_i].lock(),
			m_charaMat,
			DirectX::XMFLOAT4{ 1.0f,0.0f,1.0f,1.0f },
			DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f }
		);
	}
	
	World::Instance().RunSystem(SystemType::Draw,0.0f);

}
