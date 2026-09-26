#include "BoidWaveSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/BoidComponent.h"
#include "../../../../Components/Character/LookAngleComponent.h"
#include "../../../../Components/Character/Boss/PlatoonLeaderComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Character/Boss/BoidWaveStateComponent.h"
#include "../../../../Components/Resource/EmissiveOverrideComponent.h"
#include "../../../../InstanceResource/WormWaveResource.h"

//==============================================================================
// BoidWaveSystem
//
// ワームの体を走るウェーブ(WormWaveResource)に合わせて、ボイドの発光を書き換える。
// ウェーブを出すのは SwarmBossController、ここは受けて塗るだけ。
//
// 位置はすべて「頭からの1次元距離」で扱う。
//
//   ボイドの1次元位置 = 小隊長の distanceAlongWorm + distanceFromPlatoonLeader
//
// 後ろの項は、小隊長から見た実際の位置を進行方向へ投影したもの(毎フレーム計算)。
// 頭側が負、尾側が正。体が曲がっていても、伸ばした一本の紐の上での距離になる。
//
// ・小隊長の位置と前方は、先にまとめて引いて配列に持つ。
//   ボイド1体ごとに RefData を叩くと、4000体ぶんの飛び飛びのアクセスになるため。
// ・前方は LookAngleComponent から作る(SwarmLookSystem が進行方向へ寄せている値)。
//   速度から直に作らないのは、止まった瞬間に向きが決まらなくなるのを避けるため。
// ・発光の強さと色は、一番近いウェーブとの距離だけで決まる(重ねて明るくはしない)。
//
// ・書き込むのは自分専用の2つだけ。
//     BoidWaveStateComponent    … 小隊長からの1次元距離(計算途中の値)
//     EmissiveOverrideComponent … 発光の差し替え。ModelComponent へ写すのは
//                                 ApplyEmissiveOverrideSystem(PreDraw)
//   以前は BoidComponent と ModelComponent を直接書いていて、それらを読むだけの
//   BoidSystem / SwarmLookSystem などと依存が循環していた(Update のソートが失敗していた)。
//   BoidComponent は所属(platoonID)を読むだけなので const。
// ・どちらも SwarmBossController がボイドの生成時に付ける。持っていないボイドは光らない。
//==============================================================================
namespace
{
	// 小隊長1体ぶんの、ウェーブの計算に要るもの
	struct PlatoonAxis
	{
		Engine::ECS::Entity entity = Engine::ECS::Limits::INVALID_ENTITY;
		Math::Vector3 pos = {};				// 位置(ワールド)
		Math::Vector3 forward = {};			// 進んでいる向き(単位ベクトル)
		float distanceAlongWorm = 0.0f;		// 頭からの1次元位置
	};

	//--------------------------------------------------------------------------
	// ウェーブの帯の中での強さ(中心で1、幅の端で0)
	//
	// 直線で落とすと帯の境目が線に見えるので、両端がなだらかになる形にする
	//--------------------------------------------------------------------------
	float CalcWaveWeight(float a_distance, float a_width)
	{
		if (a_width <= 0.0f) return 0.0f;

		const float _t = 1.0f - std::clamp(std::fabs(a_distance) / a_width, 0.0f, 1.0f);
		return _t * _t * (3.0f - 2.0f * _t);
	}
}

void BoidWaveSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const BoidComponent, const LocalTransformComponent, BoidWaveStateComponent, EmissiveOverrideComponent>(
		Engine::ECS::ESystemType::Update,
		"BoidWaveSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const BoidComponent*              a_boidArray,
			const LocalTransformComponent*    a_localTRSArray,
			BoidWaveStateComponent*           a_waveStateArray,
			EmissiveOverrideComponent*        a_emissiveArray
		)
		{
			if (!a_ctx.pWorld) return;

			const WormWaveResource& _wave = a_ctx.pWorld->GetResource<WormWaveResource>();

			// ボスが居ない(誰もウェーブを回していない)間は触らない。
			// プレハブに設定された発光をそのまま残す
			if (!_wave.isActive) return;

			//------------------------------------------------------------------
			// 小隊長の位置と前方を先に集める
			//
			// 数は小隊長の数(数十)なので、ボイドごとの引き直しより線形探索のほうが速い。
			// チャンクごとに集め直しているが、1回あたりは小隊長の数ぶんなので割に合う。
			//
			// 置き場を static にしているのは、確保した領域を使い回すため。
			// システムはメインスレッドで1つずつ回る(SystemManager)ので共有してよい
			//------------------------------------------------------------------
			static std::vector<PlatoonAxis> _axisVec = {};
			_axisVec.clear();

			a_ctx.pWorld->ForEach<const ActiveTag, const PlatoonLeaderComponent,
				const LocalTransformComponent, const LookAngleComponent>(
				[](
					Engine::ECS::Chunk* a_pLeaderChunk,
					uint32_t a_leaderCount,
					const ActiveTag* a_leaderTags,
					const PlatoonLeaderComponent* a_platoonArray,
					const LocalTransformComponent* a_leaderTRSArray,
					const LookAngleComponent* a_lookArray
				)
				{
					for (uint32_t _i = 0; _i < a_leaderCount; ++_i)
					{
						PlatoonAxis _axis = {};
						_axis.entity            = a_pLeaderChunk->entityData[_i];
						_axis.pos               = a_leaderTRSArray[_i].pos;
						_axis.forward           = MakeLookForward(a_lookArray[_i]);
						_axis.distanceAlongWorm = a_platoonArray[_i].distanceAlongWorm;

						_axisVec.push_back(_axis);
					}
				}
			);

			if (_axisVec.empty()) return;

			//------------------------------------------------------------------
			// ボイドごとに1次元位置を出して、発光を決める
			//------------------------------------------------------------------
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const BoidComponent& _boid = a_boidArray[_i];
				BoidWaveStateComponent& _waveState = a_waveStateArray[_i];

				// 自分の小隊長を引く
				const PlatoonAxis* _pAxis = nullptr;
				for (const PlatoonAxis& _axis : _axisVec)
				{
					if (_axis.entity != _boid.platoonID) continue;
					_pAxis = &_axis;
					break;
				}
				if (!_pAxis) continue;

				//--------------------------------------------------------------
				// 小隊長からの1次元距離。
				// 進行方向へ投影して符号を反転させる(頭側が負、尾側が正)
				//--------------------------------------------------------------
				const Math::Vector3 _toBoid = a_localTRSArray[_i].pos - _pAxis->pos;
				_waveState.distanceFromPlatoonLeader = -_toBoid.Dot(_pAxis->forward);

				const float _posAlongWorm = _pAxis->distanceAlongWorm + _waveState.distanceFromPlatoonLeader;

				//--------------------------------------------------------------
				// 一番近いウェーブとの距離で強さを決める
				//--------------------------------------------------------------
				float _weight = 0.0f;
				for (const SwarmBossWave& _w : _wave.waves)
				{
					_weight = std::max(_weight, CalcWaveWeight(_posAlongWorm - _w.position, _wave.width));

					// 中心に届いていればこれ以上は上がらない
					if (_weight >= 1.0f) break;
				}

				// ModelComponent へは直接書かない(写すのは ApplyEmissiveOverrideSystem)
				EmissiveOverrideComponent& _emissive = a_emissiveArray[_i];
				_emissive.emissiveIntensity = std::lerp(_wave.baseIntensity, _wave.peakIntensity, _weight);
				_emissive.emissiveColor     = Math::Vector3::Lerp(_wave.baseColor, _wave.peakColor, _weight);
				_emissive.isOverride        = true;
			}
		}
	);
}
