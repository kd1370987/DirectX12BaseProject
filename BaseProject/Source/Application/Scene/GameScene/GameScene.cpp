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

void GameScene::Init()
{
}

void GameScene::RegistryComponent()
{

	// コンポーネント登録
	World::Instance().RegisterComponentType<ActiveCameraTag>("ActiveCameraTag");
	World::Instance().RegisterComponentType<CameraTag>("CameraTag");
	World::Instance().RegisterComponentType<CameraControllTag>("CameraControllTag");
	World::Instance().RegisterComponentType<PlayerControllTag>("PlayerControllTag");

	World::Instance().RegisterComponentType<CameraParamComponent>("CameraParam");
	World::Instance().RegisterComponentType<ProjMatComponent>("ProjMat");
	World::Instance().RegisterComponentType<FocusParamComponent>("FocusParam");
	World::Instance().RegisterComponentType<FollowTargetComponent>("FollowTarget");
	World::Instance().RegisterComponentType<TPSOffsetComponent>("TPSOffset");
	World::Instance().RegisterComponentType<TPSLookAngleComponent>("TPSLookAngle");

	World::Instance().RegisterComponentType<VelocityComponent>("Velocity");
	World::Instance().RegisterComponentType<GravityComponent>("Gravity");
	World::Instance().RegisterComponentType<InertiaComponent>("Inertia");

	World::Instance().RegisterComponentType<PlayerLookAngleComponent>("PlayerLookAngle");

	World::Instance().RegisterComponentType<ColliderComponent>("Col");
	World::Instance().RegisterComponentType<RayColliderComponent>("RayCol");

	World::Instance().RegisterComponentType<TRSComponent>("Transform");
	World::Instance().RegisterComponentType<WorldMatrixComponent>("WorldMatrix");

	World::Instance().RegisterComponentType<ModelComponent>("Model");
	World::Instance().RegisterComponentType<AnimatorComponent>("Anima");
	World::Instance().RegisterComponentType<SkeletonPoseComponent>("SkePose");
	World::Instance().RegisterComponentType<NodePoseComponent>("NodePose");

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
}

	

