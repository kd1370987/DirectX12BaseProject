#include "PointLightSystem.h"

#include "Application/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Light/PointLightComponent.h"

#include "Engine/Graphics/GraphicEngine.h"

//==========================================================================================
// PointLightSystem
//
// PointLightComponent を持つエンティティの位置と設定値を、
// GraphicsEngine の LightManager が持つ実体へ毎フレーム書き込む。
//
// PreDraw に置いてあるのは、GraphicsEngine::Execute() が
// 「ECSの描画フェーズ → ライトをGPUバッファへ詰め直す → レンダーグラフ実行」の順で回るため。
// これより後ろの帯に置くと、書いた値が乗るのが1フレーム遅れる。
//==========================================================================================
void PointLightSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, PointLightComponent>(
		Engine::ECS::ESystemType::PreDraw,
		"PointLightSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_worldMatArray,
			PointLightComponent* a_lightArray
		)
		{
			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
			if (!_pGE) return;

			auto* _pLightManager = _pGE->RefLightManager();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				PointLightComponent& _lightComp = a_lightArray[_i];

				// 席が無ければ取る。
				// 生成直後だけでなく、シーンを読み直した後やプレハブから複製された
				// 直後もここを通る(ハンドルは保存しないため)
				if (!_lightComp.handle.IsValid())
				{
					_lightComp.handle = _pLightManager->AllocatePL();

					// 上限に達していると無効が返る。
					// 誰かが返した次のフレームに取れるので、ここでは諦めるだけにする
					if (!_lightComp.handle.IsValid()) continue;
				}

				auto* _pLight = _pLightManager->RefLight(_lightComp.handle);
				if (!_pLight) continue;

				// 設定値は毎フレームそのまま書き写す。
				// 差分を見て省くこともできるが、エディターで触った値が
				// そのフレームで効くのと、親に付いて動く場合の追従を
				// 同じ経路でまかなえるほうが事故が少ない
				_pLight->pos = Math::Vector3::Transform(
					_lightComp.posOffset,
					a_worldMatArray[_i].worldMat
				);
				_pLight->color      = _lightComp.color;
				_pLight->brightness = _lightComp.brightness;
				_pLight->range      = _lightComp.range;
			}
		}
	);
}
