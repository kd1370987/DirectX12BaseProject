#include "CalcMatrixSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"

void CalcMatrixSystem::Init(Engine::ECS::World& a_world)
{
	// ヒエラルキーがついていない単体オブジェクトに対して最終行列を作成する
	a_world.ActiveTask<const LocalTransformComponent, WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"CalcMatrixSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const LocalTransformComponent* a_trsArray,
			WorldMatrixComponent* a_worldMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LocalTransformComponent& _trsComp = a_trsArray[_i];

				// 変更がなければ更新しない
				if (!_trsComp.isDirty) continue;

				WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];

				// 変換行列計算(スケール→回転→平行移動の順で合成)
				// 回転は正規化してから使う。エディタで直打ちした値が
				// 単位長でないとスケールが混ざるため
				_worldMatComp.worldMat = Math::Matrix::CreateTRS(
					_trsComp.pos,
					_trsComp.quat.Normalized(),
					_trsComp.scale);

				// mutubleでconstを無視している
				_trsComp.isDirty = false;
			}
		},
		Engine::ECS::Exclude<HierarchyComponent>()
	);
}