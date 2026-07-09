#include "GameManager.h"

// エンジン
#include "../../../Engine/MainEngine.h"

// シーン関係
#include "../../../Engine/Scene/SceneManager/SceneManager.h"

// ECS関係
#include "../../../Engine/ECS/World/World.h"

// コンポーネント関係
// システムフェーズタグ
#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/AwekeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/StartTag.h"
#include "Application/Components/Tag/SystemPhaseTag/ActiveTag.h"

// コンポーネント
#include "Application/Components/Tag/RenderTag/RayTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"
#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Tag/CameraControllTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "Application/Components/Camera/TPSLookAngleComponent.h"
#include "Application/Components/Force/GravityComponent.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/InertiaComponent.h"
#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Intent/MoveIntentComponent.h"
#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Resource/UIComponent.h"
#include "Application/Components/Resource/StateMachineComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"
#include "Application/Components/Persistence/NameComponent.h"
#include "Application/Components/Hierarchy/HierarchyComponent.h"
#include "Application/Components/Hierarchy/ExoskeletonAttachementComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Charactor/Robot/BoostComponent.h"
#include "Application/Components/Resource/ParticlesComponent.h"
#include "Application/Components/Camera/TPSCameraStateComponent.h"

// システム関連
#include "Application/Systems/Init/PostDeserialize/ModelFixupSystem/ModelFixupSystem.h"
#include "Application/Systems/Init/PostDeserialize/GUIDFixupSystem/GUIDFixupSystem.h"
#include "Application/Systems/Init/Awake/FollowTargetLinkSystem/FollowTargetLinkSystem.h"
#include "Application/Systems/Init/Awake/AttachmentLinkSystem/AttachmentLinkSystem.h"
#include "Application/Systems/Init/Awake/HierarchyLinkSystem/HierarchyLinkSystem.h"
#include "Application/Systems/Init/Start/CameraStartSystem/CameraStartSystem.h"
#include "Application/Systems/Init/Start/AnimationModelStartSystem/AnimationModelStartSystem.h"
#include "Application/Systems/Init/Start/RegisterCollisionWorldSystem/RegisterCollisionWorldSystem.h"
#include "Application/Systems/Init/Start/AttachmentNodeLinkSystem/AttachmentNodeLinkSystem.h"
#include "Application/Systems/Update/Input/InputMoveSystem/InputMoveSystem.h"
#include "Application/Systems/Update/Update/Rotation/RotationSystem/RotationSystem.h"
#include "Application/Systems/Update/Update/Acceleration/GravitySystem/GravitySystem.h"
#include "Application/Systems/Update/Update/Move/CharactorMovementSystem/CharactorMovementSystem.h"
#include "Application/Systems/Update/Physics/RayCollisionSystem/RayCollisionSystem.h"
#include "Application/Systems/Update/Physics/Integral/PositionIntegrationSystem/PositionIntegrationSystem.h"
#include "Application/Systems/Update/Camera/TPSSystem/TPSSystem.h"
#include "Application/Systems/Update/PostUpdate/CommitWorldMatrixSystem/CalcMatrixSystem.h"
#include "Application/Systems/Update/PostUpdate/AnimationSystem/AnimationSystem.h"
#include "Application/Systems/Update/PostUpdate/SkinningSystem/SkinningSystem.h"
#include "Application/Systems/Update/PostUpdate/CalcNodeSystem/CalcNodeSystem.h"
#include "Application/Systems/Update/PostUpdate/CalcTransformFromExoskeletonSystem/CalcTransformFromExoskeletonSystem.h"
#include "Application/Systems/Draw/PreDraw/CamSetShaderSystem/CamSetShaderSystem.h"
#include "Application/Systems/Draw/Draw/StaticObjectDrawSystem/StaticObjectDrawSystem.h"
#include "Application/Systems/Draw/Draw/DynamicObjectDrawSystem/DynamicObjectDrawSystem.h"
#include "Application/Systems/Draw/Draw/AnimationOptionalDraw/AnimationOptionalDraw.h"
#include "Application/Systems/Draw/Draw/ScreenUIDraw/ScreenUIDrawSystem.h"
#include "Application/Systems/Draw/Draw/RegisterRayWorldSystem/RegisterRayWorldSystem.h"
#include "Application/Systems/Release/AnimationMatrixFreeSystem/AnimationMatrixFreeSystem.h"
#include "Application/Systems/Draw/PostDraw/RegisterPrevWorldMatSystem/RegisterPrevWorldMatSystem.h"
#include "Application/Systems/Init/PostDeserialize/StateMachinFixupSystem/StateMachinFixupSystem.h"
#include "Application/Systems/Update/Update/StateMachinComitSystem/StateMachinComitSystem.h"
#include "Application/Systems/Update/PreUpdate/PlayerIntentSystem/PlayerIntentSystem.h"
#include "Application/Systems/Update/Animation/AnimationStateSystem/AnimationStateSystem.h"
#include "Application/Systems/Update/Update/Move/RobotBoostSystem/RobotBoostSystem.h"
#include "Application/Systems/Draw/Draw/EmittParticlesSystem/EmittParticlesSystem.h"
#include "Application/Systems/Update/Update/Particle/ParticleEmitSystem/ParticleEmitSystem.h"
#include "Application/Systems/Init/PostDeserialize/ParticleFixupSystem/ParticleFixupSystem.h"
#include "Application/Systems/Update/PreUpdate/UpdateHierarchyDepthSystem/UpdateHierarchyDepthSystem.h"
#include "Application/Systems/Update/PostUpdate/CommitHierarchyWorldMatrixSystem/CommitHierarchyWorldMatrixSystem.h"
#include "../../Systems/Draw/Draw/SkinningRegisterSystem/SkinningRegisterSystem.h"
#include "../../Systems/Release/ResourceFreeSystem/ResourceFreeSystem.h"

// リソース関係
#include "Application/InstanceResource/HierarchyResource.h"

// インプット
#include "Engine/Input/InputCollector/InputCollector.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"

// ゲームフロウ
#include "../GameFlowStateMachine/GameFlowStateMachine.h"

namespace App::Game
{
	void App::Game::GameManager::Init()
	{
		// ゲームフロウの読み込み
		m_upGameFlowMachine = std::make_unique<GameFlowStateMachine>();
		m_upGameFlowMachine->Load("Asset/Scenes/Flow/Flow.scene");

		// ワールドの初期化関数登録
		Engine::Scene::SceneManager::Instance().SetWorldInitCallback(
			[](Engine::ECS::World* a_pWorld)
			{
				// ECSにコンポーネントを登録
				a_pWorld->RegisterComponent<PostDeserializeTag>("PostDeserializeTag");
				a_pWorld->RegisterComponent<AwekeTag>("AwekeTag");
				a_pWorld->RegisterComponent<StartTag>("StartTag");
				a_pWorld->RegisterComponent<ActiveTag>("ActiveTag");
				a_pWorld->RegisterComponent<ReleaseTag>("ReleaseTag");

				a_pWorld->RegisterComponent<RayTag>("RayTag");

				a_pWorld->RegisterComponent<ActiveCameraTag>("ActiveCameraTag");
				a_pWorld->RegisterComponent<CameraTag>("CameraTag");
				a_pWorld->RegisterComponent<CameraControllTag>("CameraControllTag");
				a_pWorld->RegisterComponent<PlayerControllTag>("PlayerControllTag");

				a_pWorld->RegisterComponent<CameraParamComponent>("CameraParamComponent");
				a_pWorld->RegisterComponent<ProjMatComponent>("ProjMatComponent");
				a_pWorld->RegisterComponent<FocusParamComponent>("FocusParamComponent");
				a_pWorld->RegisterComponent<FollowTargetComponent>("FollowTargetComponent");
				a_pWorld->RegisterComponent<TPSOffsetComponent>("TPSOffsetComponent");
				a_pWorld->RegisterComponent<TPSLookAngleComponent>("TPSLookAngleComponent");
				a_pWorld->RegisterComponent<VelocityComponent>("VelocityComponent");
				a_pWorld->RegisterComponent<GravityComponent>("GravityComponent");
				a_pWorld->RegisterComponent<InertiaComponent>("InertiaComponent");
				a_pWorld->RegisterComponent<PlayerLookAngleComponent>("PlayerLookAngleComponent");
				a_pWorld->RegisterComponent<ColliderComponent>("ColliderComponent");
				a_pWorld->RegisterComponent<RayColliderComponent>("RayColliderComponent");
				a_pWorld->RegisterComponent<LocalTransformComponent>("LocalTransformComponent");
				a_pWorld->RegisterComponent<WorldMatrixComponent>("WorldMatrixComponent");
				a_pWorld->RegisterComponent<ModelComponent>("ModelComponent");
				a_pWorld->RegisterComponent<AnimatorComponent>("AnimatorComponent");
				a_pWorld->RegisterComponent<SkeletonPoseComponent>("SkeletonPoseComponent");
				a_pWorld->RegisterComponent<NodePoseComponent>("NodePoseComponent");
				a_pWorld->RegisterComponent<UIComponent>("UIComponent");
				a_pWorld->RegisterComponent<NameComponent>("NameComponent");
				a_pWorld->RegisterComponent<GUIDComponent>("GUIDComponent");
				a_pWorld->RegisterComponent<HierarchyComponent>("HierarchyComponent");
				a_pWorld->RegisterComponent<ExoskeletonAttachmentComponent>("ExoskeletonAttachmentComponent");
				a_pWorld->RegisterComponent<StateMachineComponent>("StateMachineComponent");
				a_pWorld->RegisterComponent<MoveIntentComponent>("MoveIntentComponent");
				a_pWorld->RegisterComponent<PreviousWorldMatrixComponent>("PreviousWorldMatrixComponent");
				a_pWorld->RegisterComponent<BoostComponent>("BoostComponent");
				a_pWorld->RegisterComponent<ParticlesComponent>("ParticlesComponent");
				a_pWorld->RegisterComponent<TPSCameraStateComponent>("TPSCameraStateComponent");

				// システム登録
				a_pWorld->RegisterSystem<ModelFixupSystem>();
				a_pWorld->RegisterSystem<GUIDFixupSystem>();
				a_pWorld->RegisterSystem<StateMachinFixupSystem>();
				a_pWorld->RegisterSystem<ParticleFixupSystem>();
				a_pWorld->RegisterSystem<FollowTargetLinkSystem>();
				a_pWorld->RegisterSystem<AttachmentLinkSystem>();
				a_pWorld->RegisterSystem<HierarchyLinkSystem>();
				a_pWorld->RegisterSystem<PlayerIntentSystem>();
				a_pWorld->RegisterSystem<StateMachinComitSystem>();
				a_pWorld->RegisterSystem<RegisterCollisionWorldSystem>();
				a_pWorld->RegisterSystem<CameraStartSystem>();
				a_pWorld->RegisterSystem<AnimationModelStartSystem>();
				a_pWorld->RegisterSystem<AttachmentNodeLinkSystem>();
				a_pWorld->RegisterSystem<CamSetShaderSystem>();
				a_pWorld->RegisterSystem<InputMoveSystem>();
				a_pWorld->RegisterSystem<GravitySystem>();
				a_pWorld->RegisterSystem<RotationSystem>();
				a_pWorld->RegisterSystem<AnimationStateSystem>();
				a_pWorld->RegisterSystem<AnimationSystem>();
				a_pWorld->RegisterSystem<CalcNodeSystem>();
				a_pWorld->RegisterSystem<SkinningSystem>();
				a_pWorld->RegisterSystem<PositionIntegrationSystem>();
				a_pWorld->RegisterSystem<CharactorMovementSystem>();
				a_pWorld->RegisterSystem<TPSSystem>();
				a_pWorld->RegisterSystem<CalcMatrixSystem>();
				a_pWorld->RegisterSystem<RobotBoostSystem>();
				a_pWorld->RegisterSystem<CalccTransformFromExoskeletonSystem>();
				a_pWorld->RegisterSystem<RayCollisionSystem>();
				a_pWorld->RegisterSystem<StaticObjectDrawSystem>();
				a_pWorld->RegisterSystem<DynamicObjectDrawSystem>();
				a_pWorld->RegisterSystem<AnimationOptionalDrawSystem>();
				a_pWorld->RegisterSystem<ScreenUIDrawSystem>();
				a_pWorld->RegisterSystem<RegisterRayWorldSystem>();
				a_pWorld->RegisterSystem<EmittParticleSystem>();
				a_pWorld->RegisterSystem<ParticleEmitSystem>();
				a_pWorld->RegisterSystem<AnimationMatrixFreeSystem>();
				a_pWorld->RegisterSystem<RegisterPrevWorldMatSystem>();
				a_pWorld->RegisterSystem<UpdateHierarchyDepthSystem>();
				a_pWorld->RegisterSystem<CommitHierarchyWorldMatrixSystem>();
				a_pWorld->RegisterSystem<SkinningRegisterSystem>();
				a_pWorld->RegisterSystem<ResourceFreeSystem>();

				// インスタンスデータの登録
				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Resource::StateMachinInstance>>();
				a_pWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
				a_pWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>();
				a_pWorld->AddResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();

				// シングルトンインスタンスの登録
				a_pWorld->AddResource<HierarchyResource>();

				// 初期化
				a_pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>().Init(10000);
				a_pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>().Init(10000);

				a_pWorld->GetResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>().Reserve(100);
				a_pWorld->GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();

				a_pWorld->GetResource<HierarchyResource>().isDirty = true;
			}
		);

		// キーボード
		{
			Engine::Input::InputCollector _keyboard;
			Engine::Input::InputButtonForWindows _add('T');
			_keyboard.AddButton("Add", std::make_shared<Engine::Input::InputButtonForWindows>(_add));
			Engine::Input::InputButtonForWindows _save('K');
			_keyboard.AddButton("Save", std::make_shared<Engine::Input::InputButtonForWindows>(_save));

			// 移動
			Engine::Input::InputAxisForWindows _move('W', 'D', 'S', 'A');
			_keyboard.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForWindows>(_move));
			// ジャンプ
			Engine::Input::InputButtonForWindows _jump(VK_SPACE);
			_keyboard.AddButton("Jump", std::make_shared<Engine::Input::InputButtonForWindows>(_jump));
			// ブースト
			Engine::Input::InputButtonForWindows _boost(VK_LSHIFT);
			_keyboard.AddButton("Boost", std::make_shared<Engine::Input::InputButtonForWindows>(_boost));
			// 視点
			Engine::Input::InputAxisForWindows _look(VK_UP, VK_RIGHT, VK_DOWN, VK_LEFT);
			_keyboard.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindows>(_look));

			// テスト用ボタン
			Engine::Input::InputButtonForWindows _debugCamRBUTTON(VK_RBUTTON);
			_keyboard.AddButton("FreeCamMode", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamRBUTTON));

			Engine::Input::InputButtonForWindows _debugCamUp('E');
			_keyboard.AddButton("FreeCamUp", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamUp));
			Engine::Input::InputButtonForWindows _debugCamDown('Q');
			_keyboard.AddButton("FreeCamDown", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamDown));

			// テスト用ボタン
			Engine::Input::InputButtonForWindows _test('T');
			_keyboard.AddButton("Test", std::make_shared<Engine::Input::InputButtonForWindows>(_test));

			// シーン遷移用
			Engine::Input::InputButtonForWindows _scene('R');
			_keyboard.AddButton("Scene", std::make_shared<Engine::Input::InputButtonForWindows>(_scene));

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

		// 最初のシーンを挿入
		//Engine::Scene::SceneManager::Instance().SetNextScene(
		//	Engine::GUID("b467bb54-09cf-4b59-93e9-87c7ba196050"),Engine::Scene::SceneChangeType::Puch
		//);
		Engine::GUID _initScene = m_upGameFlowMachine->Start();
		if (_initScene != Engine::DefaultGUID)
		{
			Engine::Scene::SceneManager::Instance().SetNextScene(_initScene, Engine::Scene::SceneChangeType::Puch);
		}
		else
		{
			ENGINE_ERRLOG(false,"初めのシーンが見つかりません");
		}

		// エディター関数登録
		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[&]()
			{
				if (ImGui::Begin("GameFlowEdit"))
				{
					if(m_upGameFlowMachine)
					{
						m_upGameFlowMachine->EditImGui();
					}
				}
				ImGui::End();
			}
		);
	}
	void GameManager::Update(float a_dt)
	{	
		m_upGameFlowMachine->SetTrigger("ON_START");

		if (Engine::Input::InputManager::Instance().IsPress("Scene"))
		{
			m_upGameFlowMachine->SetTrigger("ToTitle");
		}
			
		// 遷移チェック
		Engine::GUID _nextScene;
		if (m_upGameFlowMachine->Evaluate(_nextScene))
		{
			// 遷移が発生したので、指定された新しいシーンをロード！
			Engine::Scene::SceneManager::Instance().SetNextScene(_nextScene, Engine::Scene::SceneChangeType::Replace);
		}

		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartWatch("GameUpdate");
		
		// シーンマネージャーの更新
		Engine::Scene::SceneManager::Instance().Update(a_dt);

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().EndWatch("GameUpdate");
	}
	void GameManager::Draw()
	{
		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartWatch("GameDraw");

		// シーンの描画 : 描画命令を積むだけで実行はしない
		Engine::Scene::SceneManager::Instance().Draw();

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().EndWatch("GameDraw");
	}
	void GameManager::Release()
	{}
	void GameManager::FireGlobalEvent(const std::string & a_eventName)
	{}
	void GameManager::EditDraw()
	{

	}
	GameManager::GameManager()
	{}
	GameManager::~GameManager()
	{}
}