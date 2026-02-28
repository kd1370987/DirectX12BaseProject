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
	// シーン特有処理
	Event();

	// シーンのシステム処理
	World::Instance().RunSystem(SystemType::Input, a_dt);

	World::Instance().RunSystem(SystemType::PreUpdate, a_dt);

	World::Instance().RunSystem(SystemType::Update, a_dt);

	World::Instance().RunSystem(SystemType::Physics, a_dt);

	World::Instance().RunSystem(SystemType::Camera, a_dt);

	World::Instance().RunSystem(SystemType::PostUpdate, a_dt);
}

void BaseScene::Draw()
{
	World::Instance().RunSystem(SystemType::PreDraw, 0.0f);

	World::Instance().RunSystem(SystemType::Draw, 0.0f);

	RenderContext::Instance().Excute();
}

void BaseScene::RegistryComponent()
{
	// コンポーネント登録
	ECS::ComponentTypeID _id = 0;
	_id = World::Instance().RegisterComponentType<ActiveCameraTag>("ActiveCameraTag");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});
	_id = World::Instance().RegisterComponentType<CameraTag>("CameraTag");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});
	_id = World::Instance().RegisterComponentType<CameraControllTag>("CameraControllTag");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});
	_id = World::Instance().RegisterComponentType<PlayerControllTag>("PlayerControllTag");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});

	_id = World::Instance().RegisterComponentType<CameraParamComponent>("CameraParam");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"FovY",offsetof(CameraParamComponent,fovY),FielMeta::Type::Float},
		{"AspectRate",offsetof(CameraParamComponent,aspectRatio),FielMeta::Type::Float},
		{"NearClip",offsetof(CameraParamComponent,nearZ),FielMeta::Type::Float},
		{"FarClip",offsetof(CameraParamComponent,farZ),FielMeta::Type::Float}
		});
	_id = World::Instance().RegisterComponentType<ProjMatComponent>("ProjMat");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"ProjMat",offsetof(ProjMatComponent,projMat),FielMeta::Type::Matrix},
		{"ProjInvMat",offsetof(ProjMatComponent,projInvMat),FielMeta::Type::Matrix}
		});
	_id = World::Instance().RegisterComponentType<FocusParamComponent>("FocusParam");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"ForcusDistance",offsetof(FocusParamComponent,focusDistance),FielMeta::Type::Float},
		{"ForcusRange",offsetof(FocusParamComponent,forcusRange),FielMeta::Type::Float},
		{"ForcusBackRange",offsetof(FocusParamComponent,forcusBackRange),FielMeta::Type::Float},
		});
	_id = World::Instance().RegisterComponentType<FollowTargetComponent>("FollowTarget");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"TargetEntity",offsetof(FollowTargetComponent,target),FielMeta::Type::U64},
		});
	_id = World::Instance().RegisterComponentType<TPSOffsetComponent>("TPSOffset");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"x",offsetof(TPSOffsetComponent,x),FielMeta::Type::Float},
		{"y",offsetof(TPSOffsetComponent,y),FielMeta::Type::Float},
		{"z",offsetof(TPSOffsetComponent,z),FielMeta::Type::Float},
		});
	_id = World::Instance().RegisterComponentType<TPSLookAngleComponent>("TPSLookAngle");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Pitch",offsetof(TPSLookAngleComponent,Pitch),FielMeta::Type::Float},
		{"ClampPitch",offsetof(TPSLookAngleComponent,ClampPitch),FielMeta::Type::Float},
		});

	_id = World::Instance().RegisterComponentType<VelocityComponent>("Velocity");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Value",offsetof(VelocityComponent,value),FielMeta::Type::Float3},
		});
	_id = World::Instance().RegisterComponentType<GravityComponent>("Gravity");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Scale",offsetof(GravityComponent,scale),FielMeta::Type::Float},
		});
	_id = World::Instance().RegisterComponentType<InertiaComponent>("Inertia");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Value",offsetof(InertiaComponent,value),FielMeta::Type::Float},
		});

	_id = World::Instance().RegisterComponentType<PlayerLookAngleComponent>("PlayerLookAngle");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Yaw",offsetof(PlayerLookAngleComponent,Yaw),FielMeta::Type::Float},
		});

	_id = World::Instance().RegisterComponentType<ColliderComponent>("Col");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Layer",offsetof(ColliderComponent,layer),FielMeta::Type::Enum},
		{"CollideLayer",offsetof(ColliderComponent,collideLayer),FielMeta::Type::EnumFlag},
		{"IsPhysical",offsetof(ColliderComponent,isPhysical),FielMeta::Type::Bool},
		});
	_id = World::Instance().RegisterComponentType<RayColliderComponent>("RayCol");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"Length",offsetof(RayColliderComponent,length),FielMeta::Type::Float},
		{"Dir",offsetof(RayColliderComponent,dir),FielMeta::Type::Float3},
		{"Position",offsetof(RayColliderComponent,pos),FielMeta::Type::Float3},
		});

	_id = World::Instance().RegisterComponentType<TRSComponent>("Transform");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"position",offsetof(TRSComponent,pos),FielMeta::Type::Float3},
		{"rotation",offsetof(TRSComponent,quat),FielMeta::Type::Float4},
		{"scale",offsetof(TRSComponent,scale),FielMeta::Type::Float3},
		});
	_id = World::Instance().RegisterComponentType<WorldMatrixComponent>("WorldMatrix");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"WorldMat",offsetof(WorldMatrixComponent,worldMat),FielMeta::Type::Matrix}
		});

	_id = World::Instance().RegisterComponentType<ModelComponent>("Model");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"ModelID",offsetof(ModelComponent,modelID),FielMeta::Type::U32},
		{"ColorScale",offsetof(ModelComponent,colorScale),FielMeta::Type::Float4},
		{"EmissiveScale",offsetof(ModelComponent,emissiveScale),FielMeta::Type::Float3}
		});
	_id = World::Instance().RegisterComponentType<AnimatorComponent>("Anima");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"ClipID",offsetof(AnimatorComponent,clipID),FielMeta::Type::U32},
		{"Time",offsetof(AnimatorComponent,time),FielMeta::Type::Float},
		{"Speed",offsetof(AnimatorComponent,speed),FielMeta::Type::Float},
		{"IsLoop",offsetof(AnimatorComponent,isLoop),FielMeta::Type::Bool}
		});
	_id = World::Instance().RegisterComponentType<SkeletonPoseComponent>("SkePose");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});
	_id = World::Instance().RegisterComponentType<NodePoseComponent>("NodePose");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {});
	_id = World::Instance().RegisterComponentType<UIComponent>("UI");
	ImGuiContex::Instance().GetCompEdit()->Register(_id, {
		{"TexID",offsetof(UIComponent,texID),FielMeta::Type::U32},
		{"UV",offsetof(UIComponent,uvOffsetTiling),FielMeta::Type::Float4},
		{"Color",offsetof(UIComponent,color),FielMeta::Type::Float4}
		});
}

void BaseScene::RegistrySystem()
{
	// システム登録
	World::Instance().RegisterSystem<CamSetShaderSystem>();
	World::Instance().RegisterSystem<InputMoveSystem>();

	World::Instance().RegisterSystem<GravitySystem>();
	World::Instance().RegisterSystem<RotationSystem>();

	World::Instance().RegisterSystem<AnimationSystem>();
	World::Instance().RegisterSystem<CalcNodeSystem>();
	World::Instance().RegisterSystem<SkinningSystem>();
	World::Instance().RegisterSystem<PositionIntegrationSystem>();

	World::Instance().RegisterSystem<TPSSystem>();

	World::Instance().RegisterSystem<CalcMatrixSystem>();
	World::Instance().RegisterSystem<RayCollisionSystem>();

	World::Instance().RegisterSystem<SimpleDrawSystem>();
	World::Instance().RegisterSystem<AnimationOptionalDrawSystem>();
	World::Instance().RegisterSystem<ScreenUIDrawSystem>();
}

void BaseScene::RegistryEntity()
{
}
