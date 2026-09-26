#include "ApplyEmissiveOverrideSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/EmissiveOverrideComponent.h"

//==========================================================================================
// ApplyEmissiveOverrideSystem
//
// EmissiveOverrideComponent に書かれた発光を ModelComponent へ写す。
//
// ・演出側(BoidWaveSystem など)は ModelComponent を直接書かず、差し替えの値だけを置く。
//   ModelComponent は描画・物理・銃など多くのシステムが読むので、書き手をここ1つに絞っておけば
//   演出側は読み手とぶつからずに済む。
// ・PreDraw に置くのは、描画(Draw)がこのフレームの値を読めるようにするため。
//   更新系(Update〜PostUpdate)で ModelComponent を読む側は発光を使っていない。
// ・isOverride が立っていないものは触らない(プレハブの発光のまま)。
//==========================================================================================
void ApplyEmissiveOverrideSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const EmissiveOverrideComponent, ModelComponent>(
		Engine::ECS::ESystemType::PreDraw,
		"ApplyEmissiveOverrideSystem",
		[](
			Engine::ECS::Chunk*,
			uint32_t a_count,
			const Engine::ECS::SystemContext&,
			ActiveTag*,
			const EmissiveOverrideComponent* a_overrideArray,
			ModelComponent* a_modelArray
		)
		{
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const EmissiveOverrideComponent& _override = a_overrideArray[_i];
				if (!_override.isOverride) continue;

				ModelComponent& _model = a_modelArray[_i];
				_model.emissiveColor     = _override.emissiveColor;
				_model.emissiveIntensity = _override.emissiveIntensity;
			}
		}
	);
}
