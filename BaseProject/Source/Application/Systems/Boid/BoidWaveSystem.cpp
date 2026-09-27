#include "BoidWaveSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/BoidMembershipComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Boid/BoidWaveStateComponent.h"
#include "Application/Components/Render/EmissiveOverrideComponent.h"
#include "Application/InstanceResource/WormWaveResource.h"
#include "Application/InstanceResource/PlatoonAxisResource.h"

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
// ・小隊長の位置と前方は、前段の PlatoonAxisSystem が PlatoonAxisResource へまとめたものを引く。
//   以前はチャンクごとに ForEach で小隊長を集め直していた。
// ・発光の強さと色は、一番近いウェーブとの距離だけで決まる(重ねて明るくはしない)。
//
// ・書き込むのは自分専用の2つだけ。
//     BoidWaveStateComponent    … 小隊長からの1次元距離(計算途中の値)
//     EmissiveOverrideComponent … 発光の差し替え。ModelComponent へ写すのは
//                                 ApplyEmissiveOverrideSystem(PreDraw)
//   所属(BoidMembershipComponent)は読むだけ。
// ・どちらも SwarmBossController がボイドの生成時に付ける。持っていないボイドは光らない。
//==============================================================================
namespace
{
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
	// 書くのは自分のチャンクの2つだけ(小隊長の軸は読むだけ)なので、チャンクを分けてワーカーで回す
	a_world.ActiveJobTask<const BoidMembershipComponent, const LocalTransformComponent, BoidWaveStateComponent, EmissiveOverrideComponent>(
		Engine::ECS::ESystemType::Update,
		"BoidWaveSystem",
		[](
			Engine::ECS::Chunk*,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*,
			const BoidMembershipComponent*    a_memberArray,
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

			const PlatoonAxisResource& _axisRes = a_ctx.pWorld->GetResource<PlatoonAxisResource>();
			if (_axisRes.axisMap.empty()) return;

			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				// 自分の小隊長の軸
				const PlatoonAxisResource::Axis* _pAxis = _axisRes.Find(a_memberArray[_i].platoonID);
				if (!_pAxis) continue;

				//--------------------------------------------------------------
				// 小隊長からの1次元距離。
				// 進行方向へ投影して符号を反転させる(頭側が負、尾側が正)
				//--------------------------------------------------------------
				BoidWaveStateComponent& _waveState = a_waveStateArray[_i];
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
	)
	.ReadsResource<WormWaveResource, PlatoonAxisResource>();
}
