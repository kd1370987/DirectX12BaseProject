#include "BaseScene.h"

#include "Engine/ECS/World/World.h"									// ECS
#include "Engine/Editor/ECSView/ComponentEdit/ComponentEdit.h"		// エディター

// コンポーネント関連
// システムフェーズタグ
#include "../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../Components/Tag/SystemPhaseTag/AwekeTag.h"
#include "../../Components/Tag/SystemPhaseTag/StartTag.h"
#include "../../Components/Tag/SystemPhaseTag/ActiveTag.h"

// レンダータグ
#include "../../Components/Tag/RenderTag/RayTag.h"

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
#include "../../Components/Transform/LocalTransformComponent.h"
#include "../../Components/Transform/WorldMatrixComponent.h"

#include "../../Components/Intent/MoveIntentComponent.h"

#include "../../Components/Collision/Collider.h"
#include "../../Components/Collision/RayCollider.h"

#include "../../Components/Resource/ModelComponent.h"
#include "../../Components/Resource/AnimatorComponent.h"
#include "../../Components/Resource/SkeletonPoseComponent.h"
#include "../../Components/Resource/NodePoseComponent.h"
#include "../../Components/Resource/UIComponent.h"
#include "../../Components/Resource/StateMachineComponent.h"

#include "../../Components/Persistence/GUIDComponent.h"
#include "../../Components/Persistence/NameComponent.h"

#include "../../Components/Hierarchy/HierarchyComponent.h"
#include "../../Components/Hierarchy/ExoskeletonAttachementComponent.h"

// システム関連
#include "../../Systems/Init/PostDeserialize/ModelFixupSystem/ModelFixupSystem.h"
#include "../../Systems/Init/PostDeserialize/GUIDFixupSystem/GUIDFixupSystem.h"

#include "../../Systems/Init/Awake/FollowTargetLinkSystem/FollowTargetLinkSystem.h"
#include "../../Systems/Init/Awake/AttachmentLinkSystem/AttachmentLinkSystem.h"
#include "../../Systems/Init/Awake/HierarchyLinkSystem/HierarchyLinkSystem.h"

#include "../../Systems/Init/Start/CameraStartSystem/CameraStartSystem.h"
#include "../../Systems/Init/Start/AnimationModelStartSystem/AnimationModelStartSystem.h"
#include "../../Systems/Init/Start/RegisterCollisionWorldSystem/RegisterCollisionWorldSystem.h"
#include "../../Systems/Init/Start/AttachmentNodeLinkSystem/AttachmentNodeLinkSystem.h"

#include "Application/Systems/Update/Input/InputMoveSystem/InputMoveSystem.h"

#include "Application/Systems/Update/Update/Rotation/RotationSystem/RotationSystem.h"
#include "Application/Systems/Update/Update/Acceleration/GravitySystem/GravitySystem.h"
#include "../../Systems/Update/Update/Move/CharactorMovementSystem/CharactorMovementSystem.h"

#include "Application/Systems/Update/Physics/RayCollisionSystem/RayCollisionSystem.h"
#include "Application/Systems/Update/Physics/Integral/PositionIntegrationSystem/PositionIntegrationSystem.h"

#include "Application/Systems/Update/Camera/TPSSystem/TPSSystem.h"

#include "Application/Systems/Update/PostUpdate/CommitWorldMatrixSystem/CalcMatrixSystem.h"
#include "Application/Systems/Update/PostUpdate/AnimationSystem/AnimationSystem.h"
#include "Application/Systems/Update/PostUpdate/SkinningSystem/SkinningSystem.h"
#include "Application/Systems/Update/PostUpdate/CalcNodeSystem/CalcNodeSystem.h"
#include "../../Systems/Update/PostUpdate/CommitWorldMatrixFromLocalSystem/CommitWorldMatrixFromLocalSystem.h"
#include "../../Systems/Update/PostUpdate/CalcTransformFromExoskeletonSystem/CalcTransformFromExoskeletonSystem.h"

#include "Application/Systems/Draw/PreDraw/CamSetShaderSystem/CamSetShaderSystem.h"

#include "Application/Systems/Draw/Draw/SimpleDraw/SimpleDrawSystem.h"
#include "Application/Systems/Draw/Draw/AnimationOptionalDraw/AnimationOptionalDraw.h"
#include "Application/Systems/Draw/Draw/ScreenUIDraw/ScreenUIDrawSystem.h"
#include "../../Systems/Draw/Draw/RegisterRayWorldSystem/RegisterRayWorldSystem.h"

#include "../../Systems/Release/AnimationMatrixFreeSystem/AnimationMatrixFreeSystem.h"


BaseScene::BaseScene()
{
}

BaseScene::~BaseScene()
{
}

void BaseScene::Enter()
{
	SetSceneType();

	// ワールド作成
	m_upWorld = std::make_unique<Engine::ECS::World>();
	m_upWorld->Init();

	// ワールド設定
	RegistryComponent();
	RegistrySystem();
	RegistryEntity();

	// シーン初期化
	Init();

	m_upWorld->RunSystem(Engine::ECS::ESystemType::Init,0.0f);
}

void BaseScene::Exit()
{
	Release();
}

void BaseScene::Update(float a_dt)
{

	// シーンの初めに一括でエンティティを生成・削除
	// 解放処理と初期化処理も含まれているため、呼び出しはシングルスレッド限定
	m_upWorld->BegineFrame();

	// シーン特有処理
	Event();

	// シーンのシステム処理
	m_upWorld->RunSystem(Engine::ECS::ESystemType::Input, a_dt);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::PreUpdate, a_dt);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::Update, a_dt);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::Physics, a_dt);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::Camera, a_dt);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::PostUpdate, a_dt);
}

void BaseScene::Draw()
{
	m_upWorld->RunSystem(Engine::ECS::ESystemType::PreDraw, 0.0f);

	m_upWorld->RunSystem(Engine::ECS::ESystemType::Draw, 0.0f);
}

void BaseScene::RegistryComponent()
{
	// コンポーネント登録
	m_upWorld->RegisterComponent<PostDeserializeTag>("PostDeserializeTag");
	m_upWorld->RegisterComponent<AwekeTag>("AwekeTag");
	m_upWorld->RegisterComponent<StartTag>("StartTag");
	m_upWorld->RegisterComponent<ActiveTag>("ActiveTag");
	m_upWorld->RegisterComponent<ReleaseTag>("ReleaseTag");

	m_upWorld->RegisterComponent<RayTag>("RayTag");

	m_upWorld->RegisterComponent<ActiveCameraTag>("ActiveCameraTag");
	m_upWorld->RegisterComponent<CameraTag>("CameraTag");
	m_upWorld->RegisterComponent<CameraControllTag>("CameraControllTag");
	m_upWorld->RegisterComponent<PlayerControllTag>("PlayerControllTag");

	m_upWorld->RegisterComponent<CameraParamComponent>("CameraParamComponent");
	m_upWorld->RegisterComponent<ProjMatComponent>("ProjMatComponent");
	m_upWorld->RegisterComponent<FocusParamComponent>("FocusParamComponent");
	m_upWorld->RegisterComponent<FollowTargetComponent>("FollowTargetComponent");
	m_upWorld->RegisterComponent<TPSOffsetComponent>("TPSOffsetComponent");
	m_upWorld->RegisterComponent<TPSLookAngleComponent>("TPSLookAngleComponent");
	m_upWorld->RegisterComponent<VelocityComponent>("VelocityComponent");
	m_upWorld->RegisterComponent<GravityComponent>("GravityComponent");
	m_upWorld->RegisterComponent<InertiaComponent>("InertiaComponent");
	m_upWorld->RegisterComponent<PlayerLookAngleComponent>("PlayerLookAngleComponent");
	m_upWorld->RegisterComponent<ColliderComponent>("ColliderComponent");
	m_upWorld->RegisterComponent<RayColliderComponent>("RayColliderComponent");
	m_upWorld->RegisterComponent<TransformComponent>("TransformComponent");
	m_upWorld->RegisterComponent<LocalTransformComponent>("LocalTransformComponent");
	m_upWorld->RegisterComponent<WorldMatrixComponent>("WorldMatrixComponent");
	m_upWorld->RegisterComponent<ModelComponent>("ModelComponent");
	m_upWorld->RegisterComponent<AnimatorComponent>("AnimatorComponent");
	m_upWorld->RegisterComponent<SkeletonPoseComponent>("SkeletonPoseComponent");
	m_upWorld->RegisterComponent<NodePoseComponent>("NodePoseComponent");
	m_upWorld->RegisterComponent<UIComponent>("UIComponent");
	m_upWorld->RegisterComponent<NameComponent>("NameComponent");
	m_upWorld->RegisterComponent<GUIDComponent>("GUIDComponent");
	m_upWorld->RegisterComponent<HierarchyComponent>("HierarchyComponent");
	m_upWorld->RegisterComponent<ExoskeletonAttachmentComponent>("ExoskeletonAttachmentComponent");
	m_upWorld->RegisterComponent<StateMachineComponent>("StateMachineComponent");
	m_upWorld->RegisterComponent<MoveIntentComponent>("MoveIntentComponent");
}

void BaseScene::RegistrySystem()
{
	// システム登録
	m_upWorld->RegisterSystem<ModelFixupSystem>();
	m_upWorld->RegisterSystem<GUIDFixupSystem>();

	m_upWorld->RegisterSystem<FollowTargetLinkSystem>();
	m_upWorld->RegisterSystem<AttachmentLinkSystem>();
	m_upWorld->RegisterSystem<HierarchyLinkSystem>();

	m_upWorld->RegisterSystem<RegisterCollisionWorldSystem>();
	m_upWorld->RegisterSystem<CameraStartSystem>();
	m_upWorld->RegisterSystem<AnimationModelStartSystem>();
	m_upWorld->RegisterSystem<AttachmentNodeLinkSystem>();

	m_upWorld->RegisterSystem<CamSetShaderSystem>();
	m_upWorld->RegisterSystem<InputMoveSystem>();

	m_upWorld->RegisterSystem<GravitySystem>();
	m_upWorld->RegisterSystem<RotationSystem>();

	m_upWorld->RegisterSystem<AnimationSystem>();
	m_upWorld->RegisterSystem<CalcNodeSystem>();
	m_upWorld->RegisterSystem<SkinningSystem>();
	m_upWorld->RegisterSystem<PositionIntegrationSystem>();
	m_upWorld->RegisterSystem<CharactorMovementSystem>();

	m_upWorld->RegisterSystem<TPSSystem>();

	m_upWorld->RegisterSystem<CalcMatrixSystem>();
	m_upWorld->RegisterSystem<CommitWorldMatrixFromLocalSystem>();
	m_upWorld->RegisterSystem<CalccTransformFromExoskeletonSystem>();
	m_upWorld->RegisterSystem<RayCollisionSystem>();

	m_upWorld->RegisterSystem<SimpleDrawSystem>();
	m_upWorld->RegisterSystem<AnimationOptionalDrawSystem>();
	m_upWorld->RegisterSystem<ScreenUIDrawSystem>();
	m_upWorld->RegisterSystem<RegisterRayWorldSystem>();

	m_upWorld->RegisterSystem<AnimationMatrixFreeSystem>();
}

void BaseScene::RegistryEntity()
{
}
