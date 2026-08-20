#include "EffectUpdateSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Effect/EffectAssetComponent.h"

//==========================================================================================
// EffectUpdateSystem
//
// EffectAssetComponent の isPlay を見て、エフェクトの時間を進める。
//
// ・isPlay の立ち上がりで頭から再生、立ち下がりで停止。
//   制御側は毎フレーム isPlay を書くだけでよく、
//   「始まった瞬間」を自分で数えなくてよい。
// ・時間を進めた結果として、このフレームの発生数(pendingEmit)がパーツごとに決まる。
//   実際の発生要求と描画は EffectDrawSystem(Draw)が行う。
//
// 実フレーム時間(a_ctx.dt)が要るので Update フェーズで回す
// (Draw フェーズは dt = 0 のため、ここでやらないと時間が進まない)。
//==========================================================================================
void EffectUpdateSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<EffectAssetComponent>(
		Engine::ECS::ESystemType::Update,
		"EffectUpdateSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			EffectAssetComponent* a_effectArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectAssetComponent& _comp = a_effectArray[_i];

				auto* _pEffect = _pResourceManager->Ref(_comp.effectHandle);
				if (!_pEffect) continue;

				// ---- 再生 / 停止の切り替え ----
				// アセット側の実体が「再生中か」を覚えているので、
				// 要求(isPlay)との差を見れば立ち上がり・立ち下がりが分かる
				if (_comp.isPlay && !_comp.instance.isPlaying)
				{
					_pEffect->Play(_comp.instance);
					ENGINE_LOG("[DBG-Update] Play (dt=%.4f)", a_ctx.dt);
				}
				else if (!_comp.isPlay && _comp.instance.isPlaying)
				{
					_pEffect->Stop(_comp.instance);
				}

				// ---- 時間を進めて、このフレームの発生数を決める ----
				_pEffect->Update(_comp.instance, a_ctx.dt);

				// ---- 出し切ったら自分ごと消す ----
				// 解放予約だけしておく。実際に消えるのは次の BeginFrame で、
				// その前に Release フェーズが走るので借りているものは返ってから消える
				if (_comp.destroyOnFinish && _pEffect->IsFinished(_comp.instance))
				{
					ENGINE_LOG("[DBG-Update] Finish -> release (elapsed=%.3f)", _comp.instance.elapsed);
					a_ctx.pWorld->AddReleaseEntity(a_pChunk->entityData[_i]);
				}
				else if (_comp.instance.pendingEmit[0] > 0 || _comp.instance.pendingEmit[1] > 0)
				{
					ENGINE_LOG("[DBG-Update] elapsed=%.3f pending=%d/%d",
						_comp.instance.elapsed,
						_comp.instance.pendingEmit[0], _comp.instance.pendingEmit[1]);
				}
			}
		}
	);
}
