#include "SubmitDynamicColliderSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

#include "../../../Shared/HierarchyTransform/HierarchyTransform.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Collision/Collision.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

//==========================================================================================
// SubmitDynamicColliderSystem
//
// 動的レイヤーのコライダーを、現在の姿勢から worldAABB を作り直して毎フレーム submit する。
// 動的ワールドは BeginFrame でツリーごと空にされているので、ここでは push するだけでよい。
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
			const LocalTransformComponent*		// 行列は親を辿って組むのでここでは使わない
			)
		{
			auto* _pCollWorld = &a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ColliderComponent& _collComp = a_collArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// 動的レイヤー以外(静的)はここでは扱わない。
				// 弾のレイヤーも動く側なので IsDynamicLayer で見る
				if (!IsDynamicLayer(_collComp.layer)) continue;

				// ワールド行列計算
				//
				// 静的側(RegisterCollisionWorldSystem)と同じ組み方を使う。
				// 今は動くものが root 直下ばかりで親の分が抜けても表に出ないが、
				// 親付きのコライダーを1つ置いた時点で絵とずれる
				Math::Matrix _mat = App::Systems::HierarchyTransform::CalcWorldMatrix(
					*a_ctx.pWorld, a_pChunk->entityData[_i]);

				// モデル取得
				const auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_pModel) continue;

				// モデル全体のローカルAABBを計算（モデル内ノードのworldTransform込み）
				DirectX::BoundingBox _localAABB = Engine::Collision::CalcModelLocalAABB(
					_pModel,
					_collComp.shapeType.type == Engine::Collision::EShapeType::Mesh);

				// ローカルAABBをワールドに変換
				DirectX::BoundingBox _worldAABB;
				_localAABB.Transform(_worldAABB, Math::DX::Load(_mat));

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
