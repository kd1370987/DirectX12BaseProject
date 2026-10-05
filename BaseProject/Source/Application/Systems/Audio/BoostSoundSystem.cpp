#include "BoostSoundSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Movement/BoostParamsComponent.h"
#include "Application/Components/Movement/BoostIntentComponent.h"
#include "Application/Components/Movement/BoostStateComponent.h"
#include "Application/Components/Effect/EffectPlayRequestComponent.h"

//==========================================================================================
// BoostSoundSystem
//
// ブースト状態から音を鳴らす。ThrusterEffectSystem のサウンド版。
//
// 鳴らす中身は機体に付けたエフェクト(BoostAudio など)のサウンドパーツが持っているので、
// ここが伝えるのは「ブーストしているか」だけ。再生 / 停止の切り替えで次のように鳴る。
//
//   噴射に入った瞬間 -> OnPlay の単発(始動音)と OnPlay のループ(継続音)
//   噴射している間   -> ループが鳴り続ける
//   噴射が終わった後 -> ループが止まり、OnStop の単発(終了音)が1回鳴る
//
// 起動・終了の判定は「推力が出ているか」の立ち上がり/立ち下がりだけで決める。
// 入力の押下フラグを見ないので、プレイヤーでもボスでも、
// 燃料切れで落ちた場合でも同じように鳴る。
//
// 始動音だけ・継続音だけといった組み合わせも、エフェクト側のパーツの有無で表現できる。
//
// ・対象はブースターを付けている機体自身(BoostParamsComponent を持つエンティティ)。
//   ブースターの子エンティティのエフェクトは ThrusterEffectSystem が噴射の有無で切り替える。
//   肩のブースターは移動・上昇でも点火するので、ブーストの間だけ鳴らしたい音を
//   そちらに入れると鳴り方が変わってしまう。そのため音は機体自身のエフェクトに分けている。
// ・機体自身にエフェクトを付けていない相手(ボスなど)は、クエリから外れて何もしない。
// ・以前は AudioBehavior アセット(Start / Loop / End)を AudioBehaviorComponent で鳴らしていた。
//==========================================================================================
void BoostSoundSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const BoostParamsComponent, const BoostIntentComponent, const BoostStateComponent, EffectPlayRequestComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"BoostSoundSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const BoostParamsComponent* a_boostArray,
			const BoostIntentComponent* a_boostIntentArray,
			const BoostStateComponent* a_boostStateArray,
			EffectPlayRequestComponent* a_requestArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const BoostParamsComponent& _boost = a_boostArray[_i];

				// 実際に推力が出る条件。
				// 燃料切れで飛べないときに音だけ鳴らないよう、
				// RobotBoostSystem / ThrusterEffectSystem と同じ判定にしている。
				// 燃料切れで落ちたときも、そのまま終了音まで流れる
				const bool _hasFuel  = a_boostStateArray[_i].currentFuel > _boost.boostFuel;
				const bool _boosting = a_boostIntentArray[_i].isBoostIntent && _hasFuel;

				// 立ち上がり / 立ち下がりは EffectUpdateSystem が見て Play / Stop する
				a_requestArray[_i].isPlay = _boosting;
			}
		}
	);
}
