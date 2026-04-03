#include "GameScene.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

#include "../SceneManager.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

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

void GameScene::Event()
{
	if (GetAsyncKeyState('R'))
	{
		SceneManager::Instance().SetNextScene(SceneType::Title,SceneChangeType::Replace);
	}

	static bool _is = false;
	if(!_is && GetAsyncKeyState('T'))
	{
		auto _handle = Engine::Resource::ModelManager::Instnace().LoadModel(
			"Asset/Model/TestModelWhite/testModelWhite.gltf");
		Engine::Raytracing::RayEngine::Instance().RegistModel(DXSM::Matrix::Identity, _handle);
		_is = true;
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

	// エンティティ生成
	Engine::ECS::Entity _player;
	{
		Engine::ECS::Signature _sig;
		_sig.set(m_upWorld->GetCompTypeID(typeid(PlayerControllTag)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(PlayerLookAngleComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(GravityComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(VelocityComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ColliderComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(RayColliderComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(SkeletonPoseComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(AnimatorComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(NodePoseComponent)));
		_player = m_upWorld->CreateEntity(_sig);
		PlayerLookAngleComponent* _lookAng = m_upWorld->RefData<PlayerLookAngleComponent>(_player);
		_lookAng->Yaw = 0;
		ColliderComponent* _collider = m_upWorld->RefData<ColliderComponent>(_player);
		_collider->layer = Layer::DiynamicObject;
		_collider->collideLayer = Layer::StaticObject;
		RayColliderComponent* _rayCol = m_upWorld->RefData<RayColliderComponent>(_player);
		_rayCol->length = 1.0f;
		_rayCol->dir = { 0.0f,-1.0f,0.0f };
		_rayCol->pos = { 0.0f,1.0f,0.0f };
		GravityComponent* _gravity = m_upWorld->RefData<GravityComponent>(_player);
		_gravity->scale = -1.f;
		VelocityComponent* _velocity = m_upWorld->RefData<VelocityComponent>(_player);
		_velocity->value = { 0.0f,0.0f,0.0f };
		ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_player);
		_model->handle = Engine::Resource::ModelManager::Instnace().LoadModel(
			"Asset/Model/SkinMeshMan/SkinMeshMan.gltf"
		);
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_player);
		_ref->pos = { 0.0f, 3.0f, 5.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };
		SkeletonPoseComponent* _pSkePose = m_upWorld->RefData<SkeletonPoseComponent>(_player);
		for (auto& _pose : _pSkePose->palette)
		{
			_pose = DXSM::Matrix::Identity;
		}
		AnimatorComponent* _pAni = m_upWorld->RefData<AnimatorComponent>(_player);
		_pAni->clipID = ModelUtility::GetAnimationClipCount(
			*Engine::Resource::ModelManager::Instnace().GetModel(_model->handle),
			"Walk"
		);
		_pAni->time = 0.0f;
		_pAni->speed = 30.0f;
		_pAni->isLoop = true;
		NodePoseComponent* _pNodePose = m_upWorld->RefData<NodePoseComponent>(_player);
		auto* _pModel = Engine::Resource::ModelManager::Instnace().GetModel(_model->handle);
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
		_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
		auto _entity = m_upWorld->CreateEntity(_sig);

		ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_entity);
		_model->modelID = ResourceManager::Instance().GetModel("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX");
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
		_ref->pos = { 0.0f, -100.0f, 200.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };*/
	}

	// 地面
	{
		Engine::ECS::Signature _sig;
		_sig.set(m_upWorld->GetCompTypeID(typeid(ColliderComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
		auto _entity = m_upWorld->CreateEntity(_sig);
		ColliderComponent* _collider = m_upWorld->RefData<ColliderComponent>(_entity);
		_collider->layer = Layer::StaticObject;
		_collider->collideLayer = Layer::DiynamicObject;
		ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_entity);
		_model->handle = Engine::Resource::ModelManager::Instnace().LoadModel("Asset/Model/Stage/StageMap.gltf");
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
		_ref->pos = { 0.0f, 0.0f, 0.0f };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };

		Engine::Raytracing::RayEngine::Instance().RegistModel(DXSM::Matrix::Identity, _model->handle);
	}

	// カメラ
	{
		Engine::ECS::Signature _sig;
		_sig.set(m_upWorld->GetCompTypeID(typeid(ActiveCameraTag)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(CameraTag)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(CameraControllTag)));

		_sig.set(m_upWorld->GetCompTypeID(typeid(TPSLookAngleComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(FollowTargetComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(CameraParamComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(FocusParamComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ProjMatComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(TPSOffsetComponent)));

		_sig.set(m_upWorld->GetCompTypeID(typeid(VelocityComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		auto _entity = m_upWorld->CreateEntity(_sig);

		TPSLookAngleComponent* _lookAng = m_upWorld->RefData<TPSLookAngleComponent>(_entity);
		_lookAng->ClampPitch = 80.0f;
		_lookAng->Pitch = 0;

		TPSOffsetComponent* _offset = m_upWorld->RefData<TPSOffsetComponent>(_entity);
		_offset->x = 0;
		_offset->y = 2;
		_offset->z = -5;

		FollowTargetComponent* _follow = m_upWorld->RefData<FollowTargetComponent>(_entity);
		_follow->target = _player;
		CameraParamComponent* _camParam = m_upWorld->RefData<CameraParamComponent>(_entity);
		_camParam->aspectRatio = static_cast<float>(1280) / static_cast<float>(720);
		_camParam->fovY = 60.f;
		_camParam->nearZ = 0.1f;
		_camParam->farZ = 1000.0f;
		FocusParamComponent* _focusPram = m_upWorld->RefData<FocusParamComponent>(_entity);
		*_focusPram = {};
		ProjMatComponent* _projMat = m_upWorld->RefData<ProjMatComponent>(_entity);
		DirectX::XMStoreFloat4x4(&_projMat->projMat, DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(_camParam->fovY), _camParam->aspectRatio, _camParam->nearZ, _camParam->farZ)
		);
		VelocityComponent* _velocity = m_upWorld->RefData<VelocityComponent>(_entity);
		_velocity->value = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
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
					Engine::ECS::Signature _sig;
					_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
					_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
					_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
					auto _entity = m_upWorld->CreateEntity(_sig);

					ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_entity);
					_model->handle = Engine::Resource::ModelManager::Instnace().LoadModel(
						"Asset/Model/TestModelWhite/testModelWhite.gltf"
					);
					_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
					_model->emissiveScale = { 0.0f,0.0f,0.0f };
					TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
					_ref->pos = { _x * _pad,  _y * _pad, -_z * _pad };
					_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
					_ref->scale = { 1.0f,1.0f,1.0f };
				}
			}
		}
	}

	{
		Engine::ECS::Signature _sig;
		_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		_sig.set(m_upWorld->GetCompTypeID(typeid(ModelComponent)));
		auto _entity = m_upWorld->CreateEntity(_sig);

		ModelComponent* _model = m_upWorld->RefData<ModelComponent>(_entity);
		_model->handle = Engine::Resource::ModelManager::Instnace().LoadModel(
			//"Asset/Model/TEST_metarogh/MRModel.gltf"
			"Asset/Model/Test/BALL/ball.gltf"
		);
		_model->colorScale = { 1.0f,1.0f,1.0f,1.0f };
		_model->emissiveScale = { 0.0f,0.0f,0.0f };
		TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
		_ref->pos = { 0,2,0 };
		_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		_ref->scale = { 1.0f,1.0f,1.0f };

		DXSM::Matrix _worldMat = DXSM::Matrix::CreateTranslation(0,3,0);
		Engine::Raytracing::RayEngine::Instance().RegistModel(_worldMat,_model->handle);
	}

	{
		//ECS::Signature _sig;
		//_sig.set(m_upWorld->GetCompTypeID(typeid(TRSComponent)));
		//_sig.set(m_upWorld->GetCompTypeID(typeid(WorldMatrixComponent)));
		//_sig.set(m_upWorld->GetCompTypeID(typeid(UIComponent)));
		//auto _entity = m_upWorld->CreateEntity(_sig);

		//UIComponent* _ui = m_upWorld->RefData<UIComponent>(_entity);
		////GraphicResourceManager::Instance().GetTexture(_ui->texID, "Asset/Texture/Test/", "uiTest.png", TextureUse::Albedo);
		//_ui->color = { 1.0f,1.0f,1.0f,0.5f };
		////std::vector<SRVViewInit> _initVec = {};
		////ID3D12Resource* _tex = GraphicResourceManager::Instance().NGetTexture(_ui->texID)->cpResource.Get();
		////_initVec.push_back({_tex});
		////_ui->srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange(_initVec)[0];
		//_ui->texHandle = Engine::Resource::TextureManager::Instance().LoadTexture("Asset/Texture/Test/uiTest.png");
		//TRSComponent* _ref = m_upWorld->RefData<TRSComponent>(_entity);
		//_ref->pos = { 0,0,0 };
		//_ref->quat = { 0.0f,0.0f,0.0f,1.0f };
		//_ref->scale = { 0.5f,0.5f,1.0f };
		//m_upWorld->RefData<WorldMatrixComponent>(_entity)->worldMat = DXSM::Matrix::Identity;
	}

	Engine::Raytracing::RayEngine::Instance().CommitWorld();
}


	

