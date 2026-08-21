#include "BoosterEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/Robot/BoosterEffectComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Utility/EffectSpawnHelper.h"

//==========================================================================================
// BoosterEffectSystem
//
// ブースターの「置き方」と「大きさの変化」を、噴射エフェクトへ渡す。
// あわせて、ブーストダッシュを踏み込んだ瞬間のスパークを出す。
//
// ・エフェクトアセットは GUID 単位で共有されるので、取り付け位置と向きの個体差は
//   アセットには書けない。BoosterEffectComponent が持っているものを、
//   毎フレーム EffectAssetComponent の上書き欄へ写して渡す。
//
// ・大きさは2つを掛け合わせて決める。
//     (1) 吹かした瞬間の膨らみ
//         点火(isPlay の立ち上がり)とダッシュの踏み込みで burstScale まで跳ね上げ、
//         burstTime かけて baseScale へ戻す。
//     (2) ブーストダッシュ中の太らせ
//         ダッシュしている間だけ boostScale 倍。こちらは持続する。
//   (1) だけだと踏み込みの一瞬しか差が出ず、飛んでいる間は歩いている時と
//   同じ絵になる。(2) だけだと切り替わりが平坦で踏み込みの手応えが出ない。
//   点火/消火とダッシュ中かを決めるのは ThrusterEffectSystem(PreUpdate)なので、
//   ここは Update 帯に置いてそのフレームの値を見る。
//   実際に出すのは EffectDrawSystem(Draw)なので、書いた値はそのフレームに間に合う。
//
// ・戻し方を「毎フレーム一定割合で減らす」ではなく残り時間の線形にしているのは、
//   burstTime にそのまま「何秒で戻るか」が出ていた方が調整しやすいため。
//   ダッシュ中かの行き来も同じ理由で boostBlendTime の線形にしている
//   (入り切りをそのまま出すとジェットが1フレームで跳ねる)。
//
// ・スパークはジェットとは別のエフェクトなので、ジェットの大きさをいじるのではなく
//   噴射口の位置へ単発のエフェクトエンティティを出す。
//   ジェットは出っぱなしで太さしか変えられないため、「瞬間的に速度が上がった」ことを
//   見せるには、絵の違うものが一度だけ弾ける方が伝わる。
//   出したものは destroyOnFinish で自分から消えるので後片付けは要らない。
//==========================================================================================
void BoosterEffectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<BoosterEffectComponent, EffectAssetComponent>(
		Engine::ECS::ESystemType::Update,
		"BoosterEffectSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			BoosterEffectComponent*           a_boosterArray,
			EffectAssetComponent*             a_effectArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoosterEffectComponent& _booster = a_boosterArray[_i];
				EffectAssetComponent&   _effect  = a_effectArray[_i];

				//--------------------------------------------------------------
				// 噴射する向き
				//
				// 手で入れた値は長さがまちまちなので、ここで揃えておかないと
				// 受け側(EffectDrawSystem)で行列を掛けたときに長さが効いてしまう。
				// 0 ベクトルのときは触らない(既定の向きのまま出す)。
				// スパークを出す向きにも使うので、先に求めておく
				//--------------------------------------------------------------
				Math::Vector3 _dir = _booster.emitDir;
				const bool _hasDir = (_dir.LengthSquared() > 1e-8f);
				if (_hasDir)
				{
					_dir.Normalize();
				}

				//--------------------------------------------------------------
				// 点火の立ち上がりで膨らませる
				//--------------------------------------------------------------
				const bool _isPlaying = _effect.isPlay;

				if (_isPlaying && !_booster.wasPlaying)
				{
					_booster.burstTimer = _booster.burstTime;
				}
				_booster.wasPlaying = _isPlaying;

				//--------------------------------------------------------------
				// ブーストダッシュの踏み込み
				//
				// 立ち上がりで膨らみを入れ直し、スパークを1回だけ出す。
				// 移動しながらダッシュに入った場合は噴射がすでに出ているので、
				// 点火の側では拾えない。ここで入れ直すことで
				// 「歩き出し」と「踏み込み」のどちらでも同じ手応えが出る
				//--------------------------------------------------------------
				const bool _isBoosting = _booster.isBoosting;
				const bool _justBoosted = _isBoosting && !_booster.wasBoosting;
				_booster.wasBoosting = _isBoosting;

				if (_justBoosted)
				{
					_booster.burstTimer = _booster.burstTime;
				}

				// 時間を進める。消火中も戻しきってしまってよい
				// (次に点火したときはどうせ入れ直すため)
				if (_booster.burstTimer > 0.0f)
				{
					_booster.burstTimer = (std::max)(0.0f, _booster.burstTimer - a_ctx.dt);
				}

				//--------------------------------------------------------------
				// ダッシュ中かの行き来 : 0 = 通常 / 1 = ブースト中
				//--------------------------------------------------------------
				const float _boostTarget = _isBoosting ? 1.0f : 0.0f;
				if (_booster.boostBlendTime > 0.0f)
				{
					const float _step = a_ctx.dt / _booster.boostBlendTime;
					if (_booster.boostBlend < _boostTarget)
					{
						_booster.boostBlend = (std::min)(_boostTarget, _booster.boostBlend + _step);
					}
					else
					{
						_booster.boostBlend = (std::max)(_boostTarget, _booster.boostBlend - _step);
					}
				}
				else
				{
					_booster.boostBlend = _boostTarget;
				}

				//--------------------------------------------------------------
				// 大きさ : 膨らみ(burstScale -> baseScale)に、ダッシュ中の倍率を掛ける
				//--------------------------------------------------------------
				float _t = 0.0f;	// 1 = 吹かした瞬間 / 0 = 戻りきった
				if (_booster.burstTime > 0.0f)
				{
					_t = std::clamp(_booster.burstTimer / _booster.burstTime, 0.0f, 1.0f);
				}

				float _scale =
					_booster.baseScale + (_booster.burstScale - _booster.baseScale) * _t;

				// ダッシュ中の太らせは倍率で乗せる。
				// 足し算にすると baseScale を変えたときに効き具合が変わってしまう
				_scale *= 1.0f + (_booster.boostScale - 1.0f) * _booster.boostBlend;

				_effect.effectScale = _scale;

				//--------------------------------------------------------------
				// 置き方 : 噴射口の位置と向きを渡す
				//--------------------------------------------------------------
				_effect.isOverrideTransform = true;
				_effect.overridePosOffset   = _booster.posOffset;

				if (_hasDir)
				{
					_effect.overrideEmitDir = _dir;
				}

				//--------------------------------------------------------------
				// 踏み込んだ瞬間のスパーク
				//
				// 噴射口のワールド位置へ、噴射と同じ向きで単発のエフェクトを出す。
				// WorldMatrixComponent はクエリに入れず RefData で引く。
				// クエリに足すと、行列を持たないブースターが丸ごと外れて
				// 上の置き方・大きさまで書かれなくなるため。
				//
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				//--------------------------------------------------------------
				if (!_justBoosted) continue;
				if (_booster.sparkEffectGUID == Engine::DefaultGUID) continue;

				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_self)) continue;

				const auto* _pWorldMat = a_ctx.pWorld->RefData<WorldMatrixComponent>(_self);
				if (!_pWorldMat) continue;

				const Math::Matrix _ownerWorld(_pWorldMat->worldMat);

				// 位置は噴射口(posOffset)をワールドへ、向きは噴射の向きをワールドへ。
				// 向きが決まっていないときは 0 ベクトルのまま渡して、
				// アセットのパーツが持っている向きに任せる
				const Math::Vector3 _sparkPos =
					Math::Vector3::Transform(_booster.posOffset, _ownerWorld);

				Math::Vector3 _sparkDir = {};
				if (_hasDir)
				{
					_sparkDir = Math::Vector3::TransformNormal(_dir, _ownerWorld);
				}

				// 反復中なので即時生成はされない。実体化は次の BeginFrame
				App::Utility::SpawnEffectAt(
					*a_ctx.pWorld,
					_booster.sparkEffectGUID,
					_sparkPos,
					true,
					_sparkDir,
					_booster.sparkScale);
			}
		}
	);
}
