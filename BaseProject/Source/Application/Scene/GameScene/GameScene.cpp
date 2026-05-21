#include "GameScene.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../SceneManager.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "../../../Engine/Persistence/PersistenceManager/PersistenceManager.h"

// ECS関連
#include "Engine/ECS/World/World.h"

// コンポーネント関連
#include "../../Components/Tag/ActiveCameraTag.h"
#include "../../Components/Tag/CameraTag.h"
#include "../../Components/Tag/PlayerControllTag.h"
#include "../../Components/Tag/CameraControllTag.h"

#include "../../Components/Camera/CameraParamComponent.h"
#include "../../Components/Camera/FocusParamComponent.h"
#include "../../Components/Camera/ProjMatComponent.h"
#include "../../Components/Camera/FollowTargetComponent.h"
#include "../../Components/Camera/TPSOffsetComponent.h"
#include "../../Components/Camera/TPSLookAngleComponent.h"

#include "../../Components/Force/GravityComponent.h"
#include "../../Components/Force/VelocityComponent.h"
#include "../../Components/Force/InertiaComponent.h"

#include "../../Components/Charactor/Player/PlayerLookAngleComponent.h"

#include "../../Components/Transform/TransformComponent.h"
#include "../../Components/Transform/WorldMatrixComponent.h"

#include "../../Components/Collision/Collider.h"
#include "../../Components/Collision/RayCollider.h"

#include "../../Components/Resource/ModelComponent.h"
#include "../../Components/Resource/AnimatorComponent.h"
#include "../../Components/Resource/SkeletonPoseComponent.h"
#include "../../Components/Resource/NodePoseComponent.h"
#include "../../Components/Resource/UIComponent.h"

// インプット
#include "../../../Engine/Input/InputCollector/InputCollector.h"

#include "../../../Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "../../../Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "../../../Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "../../../Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "../../../Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('R'))
	{
		//SceneManager::Instance().SetNextScene(SceneType::Title,SceneChangeType::Replace);
	}

	if (Engine::Input::InputManager::Instance().IsPress("Add"))
	{
		//auto _handle = Engine::Resource::ModelManager::Instnace().LoadModel("Asset/Model/TestModelWhite/testModelWhite.gltf");
		//Engine::GUID _guid;
		//_guid.FromString("a9d483b5-5681-40ba-b45d-e1630f066516");
		//auto _handle = Engine::Resource::ModelManager::Instnace().Load(_guid);
		//Engine::Raytracing::RayEngine::Instance().RegistModel(DXSM::Matrix::Identity, _handle);
	}

	if (Engine::Input::InputManager::Instance().IsPress("Save"))
	{
		Engine::Persistence::PersistenceManager _pers = {};
		_pers.SeceneSerialize(m_upWorld.get(), "Asset/Data/Scene/GameScene_01.json");
	}
}

void GameScene::Init()
{
	// キーボード
	{
		Engine::Input::InputCollector _keyboard;
		Engine::Input::InputButtonForWindows _add('T');
		_keyboard.AddButton("Add", std::make_shared<Engine::Input::InputButtonForWindows>(_add));
		Engine::Input::InputButtonForWindows _save('K');
		_keyboard.AddButton("Save",std::make_shared<Engine::Input::InputButtonForWindows>(_save));

		// 移動
		Engine::Input::InputAxisForWindows _move('W', 'D', 'S', 'A');
		_keyboard.AddAxis("Move",std::make_shared<Engine::Input::InputAxisForWindows>(_move));

		// 視点
		Engine::Input::InputAxisForWindows _look(VK_UP, VK_RIGHT, VK_DOWN, VK_LEFT);
		_keyboard.AddAxis("Look",std::make_shared<Engine::Input::InputAxisForWindows>(_look));

		Engine::Input::InputManager::Instance().AddDevice("Keyboard", std::make_unique<Engine::Input::InputCollector>(_keyboard));
	}
	// マウス
	{
		// 視点
		Engine::Input::InputCollector _mouse;
		_mouse.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindowsMouse>());

		Engine::Input::InputManager::Instance().AddDevice("Mouse", std::make_unique<Engine::Input::InputCollector>(_mouse));
	}
	// コントローラー
	{
		//Engine::Input::InputCollector _cont;
		//_cont.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForXInput>(0,false));
		//_cont.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForXInput>(0,true));

		//Engine::Input::InputManager::Instance().AddDevice("Controller", std::make_unique<Engine::Input::InputCollector>(_cont));
	}
}

void GameScene::Release()
{
}

void GameScene::RegistryComponent()
{
	BaseScene::RegistryComponent();
}

void GameScene::RegistrySystem()
{
	BaseScene::RegistrySystem();
}

void GameScene::RegistryEntity()
{
	BaseScene::RegistryEntity();

	Engine::Persistence::PersistenceManager _pers = {};
	_pers.SeceneDeserialize(m_upWorld.get(),"Asset/Data/Scene/GameScene_01.json");

	if(false)
	{
		// テストモデル
		float _xMax = 10;
		float _yMax = 10;
		float _zMax = 1;

		float _pad = 5;

		// 高さ
		for (int _y = 0; _y < _yMax; ++_y)
		{
			// 一面
			for (int _x = 0; _x < _xMax; ++_x)
			{
				for (int _z = 0; _z < _zMax; ++_z)
				{
					Engine::ECS::Signature _sig;
					_sig.set(m_upWorld->GetCompTypeID(typeid(TransformComponent)));
					_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
					_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
					_sig.set(m_upWorld->GetCompTypeID(typeid(ColliderComponent)));
					auto _entity = m_upWorld->CreateEntity(_sig);

					ColliderComponent* _coll = m_upWorld->RefData<ColliderComponent>(_entity);
					_coll->layer = Layer::StaticObject;
					_coll->collideLayer = Layer::DiynamicObject;

					ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_entity);
					Engine::GUID _guid;
					_guid.FromString("a9d483b5-5681-40ba-b45d-e1630f066516");
					_model->handle = Engine::Resource::ModelLoader::Load(_guid);
					_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
					_model->emissiveScale = { 0.0f,0.0f,0.0f };
					_model->modelGUID = _guid;
					TransformComponent* _ref = m_upWorld->RefData<TransformComponent>(_entity);
					_ref->pos = { _x * _pad,  _y * _pad, -_z * _pad };
					_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
					_ref->scale = { 1.0f,1.0f,1.0f };
				}
			}
		}
	}
	


	return;
}


	

