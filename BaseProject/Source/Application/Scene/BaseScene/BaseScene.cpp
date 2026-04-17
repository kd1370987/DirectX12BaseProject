#include "BaseScene.h"

#include "Engine/ECS/World/World.h"									// ECS
#include "Engine/Graphics/RenderContext/RenderContext.h"			// 描画
#include "Engine/Editor/ECSView/ComponentEdit/ComponentEdit.h"		// エディター

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

#include "../../Components/Transform/TRSComponent.h"
#include "../../Components/Transform/WorldMatrixComponent.h"

#include "../../Components/Collision/Collider.h"
#include "../../Components/Collision/RayCollider.h"

#include "../../Components/Resource/ModelComponent.h"
#include "../../Components/Resource/AnimatorComponent.h"
#include "../../Components/Resource/SkeletonPoseComponent.h"
#include "../../Components/Resource/NodePoseComponent.h"
#include "../../Components/Resource/UIComponent.h"

// システム関連
#include "Application/Systems/Update/Input/InputMoveSystem/InputMoveSystem.h"

#include "Application/Systems/Update/Update/Rotation/RotationSystem/RotationSystem.h"
#include "Application/Systems/Update/Update/Acceleration/GravitySystem/GravitySystem.h"

#include "Application/Systems/Update/Physics/RayCollisionSystem/RayCollisionSystem.h"
#include "Application/Systems/Update/Physics/Integral/PositionIntegrationSystem/PositionIntegrationSystem.h"

#include "Application/Systems/Update/Camera/TPSSystem/TPSSystem.h"

#include "Application/Systems/Update/PostUpdate/CommitWorldMatrixSystem/CalcMatrixSystem.h"
#include "Application/Systems/Update/PostUpdate/AnimationSystem/AnimationSystem.h"
#include "Application/Systems/Update/PostUpdate/SkinningSystem/SkinningSystem.h"
#include "Application/Systems/Update/PostUpdate/CalcNodeSystem/CalcNodeSystem.h"

#include "Application/Systems/Draw/PreDraw/CamSetShaderSystem/CamSetShaderSystem.h"

#include "Application/Systems/Draw/Draw/SimpleDraw/SimpleDrawSystem.h"
#include "Application/Systems/Draw/Draw/AnimationOptionalDraw/AnimationOptionalDraw.h"
#include "Application/Systems/Draw/Draw/ScreenUIDraw/ScreenUIDrawSystem.h"


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

	Engine::Graphics::RenderContext::Instance().Excute();
}

void BaseScene::RegistryComponent()
{
	// コンポーネント登録
	Engine::ECS::ComponentTypeID _id = 0;
	_id = m_upWorld->RegisterComponentType<ActiveCameraTag>("ActiveCameraTag");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {});
	_id = m_upWorld->RegisterComponentType<CameraTag>("CameraTag");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {});
	_id = m_upWorld->RegisterComponentType<CameraControllTag>("CameraControllTag");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(), _id, {});
	_id = m_upWorld->RegisterComponentType<PlayerControllTag>("PlayerControllTag");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(), _id, {});

	m_upWorld->RegisterComponent<CameraParamComponent>("CameraParam");
	_id = m_upWorld->RegisterComponentType<ProjMatComponent>("ProjMat");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(), _id, {
		{"ProjMat",offsetof(ProjMatComponent,projMat),Engine::Editor::FielMeta::Type::Matrix},
		{"ProjInvMat",offsetof(ProjMatComponent,projInvMat),Engine::Editor::FielMeta::Type::Matrix}
		});
	_id = m_upWorld->RegisterComponentType<FocusParamComponent>("FocusParam");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"ForcusDistance",offsetof(FocusParamComponent,focusDistance),Engine::Editor::FielMeta::Type::Float},
		{"ForcusRange",offsetof(FocusParamComponent,forcusRange),Engine::Editor::FielMeta::Type::Float},
		{"ForcusBackRange",offsetof(FocusParamComponent,forcusBackRange),Engine::Editor::FielMeta::Type::Float},
		});
	_id = m_upWorld->RegisterComponentType<FollowTargetComponent>("FollowTarget");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"TargetEntity",offsetof(FollowTargetComponent,target),Engine::Editor::FielMeta::Type::U64},
		});
	_id = m_upWorld->RegisterComponentType<TPSOffsetComponent>("TPSOffset");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"x",offsetof(TPSOffsetComponent,x),Engine::Editor::FielMeta::Type::Float},
		{"y",offsetof(TPSOffsetComponent,y),Engine::Editor::FielMeta::Type::Float},
		{"z",offsetof(TPSOffsetComponent,z),Engine::Editor::FielMeta::Type::Float},
		});
	_id = m_upWorld->RegisterComponentType<TPSLookAngleComponent>("TPSLookAngle");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Pitch",offsetof(TPSLookAngleComponent,Pitch),Engine::Editor::FielMeta::Type::Float},
		{"ClampPitch",offsetof(TPSLookAngleComponent,ClampPitch),Engine::Editor::FielMeta::Type::Float},
		});

	_id = m_upWorld->RegisterComponentType<VelocityComponent>("Velocity");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Value",offsetof(VelocityComponent,value),Engine::Editor::FielMeta::Type::Float3},
		});
	_id = m_upWorld->RegisterComponentType<GravityComponent>("Gravity");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Scale",offsetof(GravityComponent,scale),Engine::Editor::FielMeta::Type::Float},
		});
	_id = m_upWorld->RegisterComponentType<InertiaComponent>("Inertia");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Value",offsetof(InertiaComponent,value),Engine::Editor::FielMeta::Type::Float},
		});

	_id = m_upWorld->RegisterComponentType<PlayerLookAngleComponent>("PlayerLookAngle");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Yaw",offsetof(PlayerLookAngleComponent,Yaw),Engine::Editor::FielMeta::Type::Float},
		});

	_id = m_upWorld->RegisterComponentType<ColliderComponent>("Col");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Layer",offsetof(ColliderComponent,layer),Engine::Editor::FielMeta::Type::Enum},
		{"CollideLayer",offsetof(ColliderComponent,collideLayer),Engine::Editor::FielMeta::Type::EnumFlag},
		{"IsPhysical",offsetof(ColliderComponent,isPhysical),Engine::Editor::FielMeta::Type::Bool},
		});
	_id = m_upWorld->RegisterComponentType<RayColliderComponent>("RayCol");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"Length",offsetof(RayColliderComponent,length),Engine::Editor::FielMeta::Type::Float},
		{"Dir",offsetof(RayColliderComponent,dir),Engine::Editor::FielMeta::Type::Float3},
		{"Position",offsetof(RayColliderComponent,pos),Engine::Editor::FielMeta::Type::Float3},
		});

	_id = m_upWorld->RegisterComponentType<TRSComponent>("Transform");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"position",offsetof(TRSComponent,pos),Engine::Editor::FielMeta::Type::Float3},
		{"rotation",offsetof(TRSComponent,quat),Engine::Editor::FielMeta::Type::Float4},
		{"scale",offsetof(TRSComponent,scale),Engine::Editor::FielMeta::Type::Float3},
		});
	_id = m_upWorld->RegisterComponentType<WorldMatrixComponent>("WorldMatrix");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"WorldMat",offsetof(WorldMatrixComponent,worldMat),Engine::Editor::FielMeta::Type::Matrix}
		});

	_id = m_upWorld->RegisterComponentType<ModelComponent>("Model");
	//Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
	//	{"ModelID",offsetof(ModelComponent,modelID),Engine::Editor::FielMeta::Type::U32},
	//	{"ColorScale",offsetof(ModelComponent,colorScale),Engine::Editor::FielMeta::Type::Float4},
	//	{"EmissiveScale",offsetof(ModelComponent,emissiveScale),Engine::Editor::FielMeta::Type::Float3}
	//	});
	Engine::Editor::MainEditor::Instance().GetCompEdit()->RegisterFunc(RefWorld(),_id,ModelComponent::GetFuncMeta());
	_id = m_upWorld->RegisterComponentType<AnimatorComponent>("Anima");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"ClipID",offsetof(AnimatorComponent,clipID),Engine::Editor::FielMeta::Type::U32},
		{"Time",offsetof(AnimatorComponent,time),Engine::Editor::FielMeta::Type::Float},
		{"Speed",offsetof(AnimatorComponent,speed),Engine::Editor::FielMeta::Type::Float},
		{"IsLoop",offsetof(AnimatorComponent,isLoop),Engine::Editor::FielMeta::Type::Bool}
		});
	_id = m_upWorld->RegisterComponentType<SkeletonPoseComponent>("SkePose");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {});
	_id = m_upWorld->RegisterComponentType<NodePoseComponent>("NodePose");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {});
	_id = m_upWorld->RegisterComponentType<UIComponent>("UI");
	Engine::Editor::MainEditor::Instance().GetCompEdit()->Register(RefWorld(),_id, {
		{"TexID",offsetof(UIComponent,texID),Engine::Editor::FielMeta::Type::U32},
		{"UV",offsetof(UIComponent,uvOffsetTiling),Engine::Editor::FielMeta::Type::Float4},
		{"Color",offsetof(UIComponent,color),Engine::Editor::FielMeta::Type::Float4}
		});
}

void BaseScene::RegistrySystem()
{
	// システム登録
	m_upWorld->RegisterSystem<CamSetShaderSystem>();
	m_upWorld->RegisterSystem<InputMoveSystem>();

	m_upWorld->RegisterSystem<GravitySystem>();
	m_upWorld->RegisterSystem<RotationSystem>();

	m_upWorld->RegisterSystem<AnimationSystem>();
	m_upWorld->RegisterSystem<CalcNodeSystem>();
	m_upWorld->RegisterSystem<SkinningSystem>();
	m_upWorld->RegisterSystem<PositionIntegrationSystem>();

	m_upWorld->RegisterSystem<TPSSystem>();

	m_upWorld->RegisterSystem<CalcMatrixSystem>();
	m_upWorld->RegisterSystem<RayCollisionSystem>();

	m_upWorld->RegisterSystem<SimpleDrawSystem>();
	m_upWorld->RegisterSystem<AnimationOptionalDrawSystem>();
	m_upWorld->RegisterSystem<ScreenUIDrawSystem>();
}

void BaseScene::RegistryEntity()
{
}
