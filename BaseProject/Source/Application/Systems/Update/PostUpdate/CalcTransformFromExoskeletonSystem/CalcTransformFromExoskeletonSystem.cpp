#include "CalcTransformFromExoskeletonSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Hierarchy/ExoskeletonAttachementComponent.h"
#include "Application/Components/Transform/TransformComponent.h"
void CalccTransformFromExoskeletonSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ExoskeletonAttachmentComponent, TransformComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ExoskeletonAttachmentComponent* a_exoArray,
			TransformComponent* a_transArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ExoskeletonAttachmentComponent& _exoComp = a_exoArray[_i];
				TransformComponent& _trsComp = a_transArray[_i];

				// 親のトランスフォームを取得
				auto* _pParentTrans = a_world.RefData<TransformComponent>(_exoComp.parentID);
				if (!_pParentTrans) continue;

				// 親の回転にオフセットをかける
				_trsComp.quat = _pParentTrans->quat;

				// オフセット位置を親の回転で回転させる
				DXSM::Vector3 _rotatedOffset = DXSM::Vector3::Transform(_exoComp.offsetPosition,_pParentTrans->quat);

				// 回転させたオフセットを親の現在位置に足す
				_trsComp.pos = DXSM::Vector3(_pParentTrans->pos) + _rotatedOffset;
			}
		}
	);
}
