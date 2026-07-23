#include "SubmitDynamicColliderSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

//==========================================================================================
// SubmitDynamicColliderSystem
//
// 動的レイヤーのコライダーを、現在の姿勢から worldAABB を作り直して毎フレーム submit する。
// 動的ワールドは BegineFrame でツリーごと空にされているので、ここでは push するだけでよい。
//==========================================================================================
void SubmitDynamicColliderSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ColliderComponent, const ModelComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"SubmitDynamicColliderSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ColliderComponent* a_collArray,
			const ModelComponent* a_modelArray,
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();
			if (!_pCollWorld) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ColliderComponent& _collComp = a_collArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const LocalTransformComponent& _transComp = a_transArray[_i];

				// 動的レイヤー以外(静的)はここでは扱わない
				if (_collComp.layer != Layer::DiynamicObject) continue;

				// ワールド行列計算
				DXSM::Matrix _tMat = DXSM::Matrix::CreateTranslation(_transComp.pos);
				DXSM::Matrix _rMat = DXSM::Matrix::CreateFromQuaternion(_transComp.quat);
				DXSM::Matrix _sMat = DXSM::Matrix::CreateScale(_transComp.scale);
				DXSM::Matrix _mat = _sMat * _rMat * _tMat;

				// モデル取得
				const auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// モデル全体のローカルAABBを計算
				DirectX::BoundingBox _localAABB = {};
				const auto& _meshHandles = _pModel->GetMeshHandles();
				if (!_meshHandles.empty())
				{
					const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandles[0]);
					if (!_pMesh) continue;

					// 最初の一個目で初期化
					_localAABB = _pMesh->GetMetaData().aabb;

					for (size_t _m = 1; _m < _meshHandles.size(); ++_m)
					{
						const auto* _pSubMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandles[_m]);
						if (!_pSubMesh) continue;

						const auto& _meta = _pSubMesh->GetMetaData();
						DirectX::BoundingBox::CreateMerged(_localAABB, _localAABB, _meta.aabb);
					}
				}

				// ローカルAABBをワールドに変換
				DirectX::BoundingBox _worldAABB;
				_localAABB.Transform(_worldAABB, _mat);

				// インスタンスを組んで動的ワールドへ submit
				Engine::Collision::CollisionInstance _inst = {};
				_inst.entity = a_pChunk->entityData[_i];
				_inst.collShape = _collComp.shapeType;

				// メッシュ形状ならモデルのハンドルも登録
				if (_inst.collShape.type == Engine::Collision::EShapeType::Mesh)
				{
					_inst.collShape.modelHandle = _modelComp.handle;
				}
				_inst.worldMat = _mat;
				_inst.worldAABB = _worldAABB;
				_inst.layer = static_cast<uint32_t>(_collComp.layer);

				_pCollWorld->AllcateDynamicEntity(_inst);
			}
		}
	);
}
