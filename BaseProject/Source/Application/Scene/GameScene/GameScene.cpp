#include "GameScene.h"

#include "Application/App.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"


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

// ImGuiEdit
#include "Engine/Editor/ECSView/ComponentEdit/ComponentEdit.h"

void GameScene::Init()
{
}

void GameScene::RegistryComponent()
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

void GameScene::RegistrySystem()
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

void GameScene::RegistryEntity()
{
	// エンティティ生成
	ECS::Entity _player;
	{
		ECS::Signature _sig;
		_sig.set(World::Instance().GetCompTypeID(typeid(PlayerControllTag)));
		_sig.set(World::Instance().GetCompTypeID(typeid(PlayerLookAngleComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(GravityComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(VelocityComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(ColliderComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(RayColliderComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(SkeletonPoseComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(AnimatorComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(NodePoseComponent)));
		_player = World::Instance().CreateEntity(_sig);
		PlayerLookAngleComponent* _lookAng = World::Instance().RefData<PlayerLookAngleComponent>(_player);
		_lookAng->Yaw = 0;
		ColliderComponent* _collider = World::Instance().RefData<ColliderComponent>(_player);
		_collider->layer = Layer::DiynamicObject;
		_collider->collideLayer = Layer::StaticObject;
		RayColliderComponent* _rayCol = World::Instance().RefData<RayColliderComponent>(_player);
		_rayCol->length = 1.0f;
		_rayCol->dir = { 0.0f,-1.0f,0.0f };
		_rayCol->pos = { 0.0f,1.0f,0.0f };
		GravityComponent* _gravity = World::Instance().RefData<GravityComponent>(_player);
		_gravity->scale = -9.f;
		VelocityComponent* _velocity = World::Instance().RefData<VelocityComponent>(_player);
		_velocity->value = { 0.0f,0.0f,0.0f };
		ModelComponent* _model = World::Instance().RefData<ModelComponent>(_player);
		//_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/Man/scene.gltf");
		//_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/Robot/Robot.gltf");
		_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/SkinMeshMan/SkinMeshMan.gltf");
		//_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/TreasureBox/TreasureBox.gltf");
		//_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/tank/tank.gltf");
		//_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/TestModelWhite/testModelWhite.gltf");
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_player);
		_ref->pos = { 0.0f, 3.0f, 5.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };
		SkeletonPoseComponent* _pSkePose = World::Instance().RefData<SkeletonPoseComponent>(_player);
		for (auto& _pose : _pSkePose->palette)
		{
			_pose = DXSM::Matrix::Identity;
		}
		AnimatorComponent* _pAni = World::Instance().RefData<AnimatorComponent>(_player);
		_pAni->clipID = ModelUtility::GetAnimationClipCount(*GraphicResourceManager::Instance().NGetModel(_model->modelID),"Walk");
		_pAni->time = 0.0f;
		_pAni->speed = 30.0f;
		_pAni->isLoop = true;
		NodePoseComponent* _pNodePose = World::Instance().RefData<NodePoseComponent>(_player);
		auto* _pModel = GraphicResourceManager::Instance().NGetModel(_model->modelID);
		_pNodePose->nodeCount = static_cast<uint16_t>(_pModel->originalNodes.size());
		for (int _i = 0; _i < MAX_NODEINDEX; ++_i)
		{
			_pNodePose->local[_i] = DXSM::Matrix::Identity;
			_pNodePose->world[_i] = DXSM::Matrix::Identity;
		}
		for (int _i = 0; _i < static_cast<int>(_pNodePose->nodeCount); ++_i)
		{
			_pNodePose->local[_i] = _pModel->originalNodes[_i].localTransform;
			_pNodePose->world[_i] = _pModel->originalNodes[_i].worldTransform;
		}
	}

	{
		/*ECS::Signature _sig;
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
		_ref->scale = { 1.0f,1.0f,1.0f };*/
	}

	// 地面
	{
		ECS::Signature _sig;
		_sig.set(World::Instance().GetCompTypeID(typeid(ColliderComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
		auto _entity = World::Instance().CreateEntity(_sig);
		ColliderComponent* _collider = World::Instance().RefData<ColliderComponent>(_entity);
		_collider->layer = Layer::StaticObject;
		_collider->collideLayer = Layer::DiynamicObject;
		ModelComponent* _model = World::Instance().RefData<ModelComponent>(_entity);
		_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/Stage/StageMap.gltf");
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
		_ref->pos = { 0.0f, 0.0f, 0.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };
	}

	// カメラ
	{
		ECS::Signature _sig;
		_sig.set(World::Instance().GetCompTypeID(typeid(ActiveCameraTag)));
		_sig.set(World::Instance().GetCompTypeID(typeid(CameraTag)));
		_sig.set(World::Instance().GetCompTypeID(typeid(CameraControllTag)));

		_sig.set(World::Instance().GetCompTypeID(typeid(TPSLookAngleComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(FollowTargetComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(CameraParamComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(FocusParamComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(ProjMatComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(TPSOffsetComponent)));

		_sig.set(World::Instance().GetCompTypeID(typeid(VelocityComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
		auto _entity = World::Instance().CreateEntity(_sig);

		TPSLookAngleComponent* _lookAng = World::Instance().RefData<TPSLookAngleComponent>(_entity);
		_lookAng->ClampPitch = 80.0f;
		_lookAng->Pitch = 0;

		TPSOffsetComponent* _offset = World::Instance().RefData<TPSOffsetComponent>(_entity);
		_offset->x = 0;
		_offset->y = 2;
		_offset->z = -5;

		FollowTargetComponent* _follow = World::Instance().RefData<FollowTargetComponent>(_entity);
		_follow->target = _player;
		CameraParamComponent* _camParam = World::Instance().RefData<CameraParamComponent>(_entity);
		_camParam->aspectRatio = static_cast<float>(1280) / static_cast<float>(720);
		_camParam->fovY = 60.f;
		_camParam->nearZ = 0.1f;
		_camParam->farZ = 1000.0f;
		FocusParamComponent* _focusPram = World::Instance().RefData<FocusParamComponent>(_entity);
		*_focusPram = {};
		ProjMatComponent* _projMat = World::Instance().RefData<ProjMatComponent>(_entity);
		DirectX::XMStoreFloat4x4(&_projMat->projMat, DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(_camParam->fovY), _camParam->aspectRatio, _camParam->nearZ, _camParam->farZ)
		);
		VelocityComponent* _velocity = World::Instance().RefData<VelocityComponent>(_entity);
		_velocity->value = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
		_ref->pos = { 0.0f, 5.0f, -10.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };
	}

	{
		// テストモデル
		float _xMax = 5;
		float _yMax = 1;
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
					ECS::Signature _sig;
					_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
					_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
					_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
					auto _entity = World::Instance().CreateEntity(_sig);

					ModelComponent* _model = World::Instance().RefData<ModelComponent>(_entity);
					_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/TestModelWhite/testModelWhite.gltf");
					_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
					_model->emissiveScale = { 0.0f,0.0f,0.0f };
					TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
					_ref->pos = { _x * _pad,  _y * _pad, -_z * _pad };
					_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
					_ref->scale = { 1.0f,1.0f,1.0f };
				}
			}
		}
	}

	{
		ECS::Signature _sig;
		_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(ModelComponent)));
		auto _entity = World::Instance().CreateEntity(_sig);

		ModelComponent* _model = World::Instance().RefData<ModelComponent>(_entity);
		_model->modelID = GraphicResourceManager::Instance().GetModel("Asset/Model/TEST_metarogh/MRModel.gltf");
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
		_ref->pos = { 0,2,0 };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };
	}

	{
		ECS::Signature _sig;
		_sig.set(World::Instance().GetCompTypeID(typeid(TRSComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(World::Instance().GetCompTypeID(typeid(UIComponent)));
		auto _entity = World::Instance().CreateEntity(_sig);

		UIComponent* _ui = World::Instance().RefData<UIComponent>(_entity);
		GraphicResourceManager::Instance().GetTexture(_ui->texID, "Asset/Texture/Test/", "uiTest.png", TextureUse::Albedo);
		_ui->color = { 1.0f,1.0f,1.0f,0.5f };
		std::vector<SRVViewInit> _initVec = {};
		ID3D12Resource* _tex = GraphicResourceManager::Instance().NGetTexture(_ui->texID)->cpResource.Get();
		_initVec.push_back({_tex});
		_ui->srvRange = DescriptorHeapManager::Instance().AllocateSRVRange(_initVec);
		TRSComponent* _ref = World::Instance().RefData<TRSComponent>(_entity);
		_ref->pos = { 0,0,0 };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 0.5f,0.5f,1.0f };
		World::Instance().RefData<WorldMatrixComponent>(_entity)->worldMat = DXSM::Matrix::Identity;
	}
}

	

