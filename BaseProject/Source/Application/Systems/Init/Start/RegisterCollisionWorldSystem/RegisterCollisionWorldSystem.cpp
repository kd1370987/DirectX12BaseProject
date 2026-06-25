#include "RegisterCollisionWorldSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/TransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void RegisterCollisionWorldSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<ColliderComponent, const ModelComponent, const TransformComponent>(
		Engine::ECS::ESystemType::Start,
		"RegisterCollisionWorldSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			StartTag* a_startTag,
			ColliderComponent* a_collArray,
			const ModelComponent* a_modelArray,
			const TransformComponent* a_transArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderComponent& _collComp = a_collArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const TransformComponent& _transComp = a_transArray[_i];

				// ワールド行列計算
				DXSM::Matrix _mat = {};
				DXSM::Matrix _tMat = DXSM::Matrix::CreateTranslation(_transComp.pos);
				DXSM::Matrix _rMat = DXSM::Matrix::CreateFromQuaternion(_transComp.quat);
				DXSM::Matrix _sMat = DXSM::Matrix::CreateScale(_transComp.scale);
				_mat = _sMat * _rMat * _tMat;

				// モデルのAABB計算
				const auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;
				
				// モデル全体のローカルAABBを計算
				DirectX::BoundingBox _localAABB = {};
				const auto& _meshHandles = _pModel->GetMeshHandles();
				if (!_meshHandles.empty())
				{
					// メッシュ取得
					const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandles[0]);

					// 最初の一個目で初期化
					_localAABB = _pMesh->GetMetaData().aabb;

					for (size_t _m = 1; _m < _meshHandles.size(); ++_m)
					{
						// メッシュ取得
						const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandles[0]);
						if (!_pMesh) continue;

						const auto& _meta = _pMesh->GetMetaData();
						DirectX::BoundingBox::CreateMerged(_localAABB, _localAABB, _meta.aabb);
					}
				}

				// ローカルAABBをワールドに変換
				DirectX::BoundingBox _worldAABB;
				_localAABB.Transform(_worldAABB,_mat);

				// コリジョンワールドの取得
				auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

				// コリジョンワールドに登録
				Engine::Collision::CollisionInstance _inst = {};
				_inst.entity = a_pChunk->entityData[_i];
				_inst.modelHandle = _modelComp.handle;
				_inst.worldMat = _mat;
				_inst.worldAABB = _worldAABB;
				_collComp.collWorldHandle = _pCollWorld->AllcateStaticEntity(_inst);
				
			}
		});
}
