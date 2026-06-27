#include "BaseScene.h"

#include "Engine/ECS/World/World.h"									// ECS
#include "Engine/Editor/ECSView/ComponentEdit/ComponentEdit.h"		// エディター

// コンポーネント関連
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

// リソース関係
#include "Application/InstanceResource/HierarchyResource.h"

namespace Engine::Scene
{
	BaseScene::BaseScene()
	{}

	BaseScene::~BaseScene()
	{}

	void BaseScene::Enter()
	{
		SetSceneType();

		// ワールド作成
		m_upWorld = std::make_unique<Engine::ECS::World>();
		m_upWorld->Init();

		// ワールド設定
		RegistryResource();
		RegistryComponent();
		RegistrySystem();
		RegistryEntity();

		// シーン初期化
		Init();

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Init, 0.0f);
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

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Animation, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Camera, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostUpdate, a_dt);
	}

	void BaseScene::Draw()
	{
		m_upWorld->RunSystem(Engine::ECS::ESystemType::PreDraw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Draw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostDraw, 0.0f);
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
		m_upWorld->RegisterComponent<PreviousWorldMatrixComponent>("PreviousWorldMatrixComponent");
		m_upWorld->RegisterComponent<BoostComponent>("BoostComponent");
		m_upWorld->RegisterComponent<ParticlesComponent>("ParticlesComponent");
		m_upWorld->RegisterComponent<TPSCameraStateComponent>("TPSCameraStateComponent");
	}

	void BaseScene::RegistrySystem()
	{
		// システム登録
		m_upWorld->RegisterSystem<ModelFixupSystem>();
		m_upWorld->RegisterSystem<GUIDFixupSystem>();
		m_upWorld->RegisterSystem<StateMachinFixupSystem>();
		m_upWorld->RegisterSystem<ParticleFixupSystem>();
		m_upWorld->RegisterSystem<FollowTargetLinkSystem>();
		m_upWorld->RegisterSystem<AttachmentLinkSystem>();
		m_upWorld->RegisterSystem<HierarchyLinkSystem>();
		m_upWorld->RegisterSystem<PlayerIntentSystem>();
		m_upWorld->RegisterSystem<StateMachinComitSystem>();
		m_upWorld->RegisterSystem<RegisterCollisionWorldSystem>();
		m_upWorld->RegisterSystem<CameraStartSystem>();
		m_upWorld->RegisterSystem<AnimationModelStartSystem>();
		m_upWorld->RegisterSystem<AttachmentNodeLinkSystem>();
		m_upWorld->RegisterSystem<CamSetShaderSystem>();
		m_upWorld->RegisterSystem<InputMoveSystem>();
		m_upWorld->RegisterSystem<GravitySystem>();
		m_upWorld->RegisterSystem<RotationSystem>();
		m_upWorld->RegisterSystem<AnimationStateSystem>();
		m_upWorld->RegisterSystem<AnimationSystem>();
		m_upWorld->RegisterSystem<CalcNodeSystem>();
		m_upWorld->RegisterSystem<SkinningSystem>();
		m_upWorld->RegisterSystem<PositionIntegrationSystem>();
		m_upWorld->RegisterSystem<CharactorMovementSystem>();
		m_upWorld->RegisterSystem<TPSSystem>();
		m_upWorld->RegisterSystem<CalcMatrixSystem>();
		m_upWorld->RegisterSystem<RobotBoostSystem>();
		m_upWorld->RegisterSystem<CalccTransformFromExoskeletonSystem>();
		m_upWorld->RegisterSystem<RayCollisionSystem>();
		m_upWorld->RegisterSystem<StaticObjectDrawSystem>();
		m_upWorld->RegisterSystem<DynamicObjectDrawSystem>();
		m_upWorld->RegisterSystem<AnimationOptionalDrawSystem>();
		m_upWorld->RegisterSystem<ScreenUIDrawSystem>();
		m_upWorld->RegisterSystem<RegisterRayWorldSystem>();
		m_upWorld->RegisterSystem<EmittParticleSystem>();
		m_upWorld->RegisterSystem<ParticleEmitSystem>();
		m_upWorld->RegisterSystem<AnimationMatrixFreeSystem>();
		m_upWorld->RegisterSystem<RegisterPrevWorldMatSystem>();
		m_upWorld->RegisterSystem<UpdateHierarchyDepthSystem>();
		m_upWorld->RegisterSystem<CommitHierarchyWorldMatrixSystem>();
	}

	void BaseScene::RegistryEntity()
	{}

	void BaseScene::RegistryResource()
	{
		// インスタンスデータの登録
		m_upWorld->AddResource<Engine::Pool::ItemPool<Engine::Resource::StateMachinInstance>>();
		m_upWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
		m_upWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();

		// シングルトンインスタンスの登録
		m_upWorld->AddResource<HierarchyResource>();

		// 初期化
		m_upWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>().Init(10000);
		m_upWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>().Init(10000);

		m_upWorld->GetResource<HierarchyResource>().isDirty = true;
	}
}