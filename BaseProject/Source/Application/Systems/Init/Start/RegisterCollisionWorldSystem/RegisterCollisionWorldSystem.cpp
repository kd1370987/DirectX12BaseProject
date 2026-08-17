#include "RegisterCollisionWorldSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Collision/Collision.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void RegisterCollisionWorldSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<ColliderComponent, const ModelComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Start,
		"RegisterCollisionWorldSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag* a_startTag,
			ColliderComponent* a_collArray,
			const ModelComponent* a_modelArray,
			const LocalTransformComponent* a_transArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderComponent& _collComp = a_collArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const LocalTransformComponent& _transComp = a_transArray[_i];

				// エンティティが、ダイナミックレイヤーならスキップ
				if (_collComp.layer == Layer::DiynamicObject) continue;

				// ワールド行列計算
				Math::Matrix _mat = {};
				Math::Matrix _tMat = Math::Matrix::CreateTranslation(_transComp.pos);
				Math::Matrix _rMat = Math::Matrix::CreateFromQuaternion(_transComp.quat);
				Math::Matrix _sMat = Math::Matrix::CreateScale(_transComp.scale);
				_mat = _sMat * _rMat * _tMat;

				// モデルのAABB計算
				const auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_pModel) continue;
				
				// モデル全体のローカルAABBを計算（モデル内ノードのworldTransform込み）
				DirectX::BoundingBox _localAABB = Engine::Collision::CalcModelLocalAABB(
					_pModel,
					_collComp.shapeType.type == Engine::Collision::EShapeType::Mesh);

				// ローカルAABBをワールドに変換
				DirectX::BoundingBox _worldAABB;
				_localAABB.Transform(_worldAABB, Math::DX::Load(_mat));

				// コリジョンワールドの取得
				auto* _pCollWorld = a_ctx.pServices->pMainEngine->RefCollisionWorld();

				// コリジョンワールドに登録
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
				_collComp.collWorldHandle = _pCollWorld->AllcateStaticEntity(_inst);
				
			}
		});
}
