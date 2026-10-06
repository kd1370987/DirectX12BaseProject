#include "BoidSteeringSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/BoidSteeringParamsComponent.h"
#include "Application/Components/Boid/BoidMembershipComponent.h"
#include "Application/Components/Boid/BoidTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"

#include "Application/InstanceResource/BoidSnapshotResource.h"

namespace App::System
{
	//==============================================================================
	// BoidSteeringSystem
	//
	// 群れの操舵(分離・整列・結合・目標への追従)から目標速度を作る。
	// 近傍は前段の BoidSnapshotSystem が作った写し(BoidSnapshotResource)から引く。
	//
	// ・書くのは自分の DesiredVelocityComponent だけ。位置を進めるのは積分(Physics)。
	// ・自分のチャンクだけを書き、写しは読むだけなので、チャンクを分けてワーカーで回す。
	//   以前の BoidSystem は写しと操舵を1つのジョブで回していて、4000体が1本に載っていた。
	// ・写しは速度を読み、こちらは速度を書くので、読み書きだけでは向きが決まらない
	//   (写し → 操舵 の順は After で決める)。
	// ・PreUpdate 帯(HomingSystem と同じ理由)。Update 帯には LockOnRotationSystem など
	//   「Velocity を読んで LocalTransform を書く」ものがいて、同じ帯に置くと依存が循環する。
	// ・小隊の中は O(N^2) で近傍を探す。小隊が大きくなったら格子などへ置き換える想定。
	//==============================================================================
	void BoidSteeringSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveJobTask<
			const Component::BoidSteeringParamsComponent,
			const Component::BoidMembershipComponent,
			const Component::BoidTargetComponent,
			const Component::LocalTransformComponent,
			Component::DesiredVelocityComponent>(
			Engine::ECS::ESystemType::PreUpdate,
			"BoidSteeringSystem",
			[](
				Engine::ECS::Chunk*,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag*,
				const Component::BoidSteeringParamsComponent* a_paramsArray,
				const Component::BoidMembershipComponent* a_memberArray,
				const Component::BoidTargetComponent* a_targetArray,
				const Component::LocalTransformComponent* a_trsArray,
				Component::DesiredVelocityComponent* a_velArray
			)
			{
				const InstanceResource::BoidSnapshotResource& _snapshot = a_ctx.pWorld->GetResource<InstanceResource::BoidSnapshotResource>();

				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const Component::BoidSteeringParamsComponent& _params = a_paramsArray[_i];
					const Component::LocalTransformComponent& _trsComp = a_trsArray[_i];
					Component::DesiredVelocityComponent& _velComp = a_velArray[_i];

					Math::Vector3 _separation = {};	// すべての反発を合成する
					Math::Vector3 _alignment = {};	// 周囲の進行ベクトル
					Math::Vector3 _cohesion = {};	// ボイドの中心

					uint32_t _neighborCount = 0;

					// 同じ小隊のボイドを検索する。小隊に属していなければ近傍は無し(Seek だけ効く)
					for (const InstanceResource::BoidSnapshotResource::Entry& _other : _snapshot.Find(a_memberArray[_i].platoonID))
					{
						const Math::Vector3 _offset = _trsComp.pos - _other.position;
						const float _distanceSquared = _offset.LengthSquared();

						// 同一座標(自分自身を含む)は方向を求められないので無視
						if (_distanceSquared <= 0.000001f) continue;
						const float _distance = std::sqrt(_distanceSquared);

						//----------------------------------------------------------
						// Separation : 近すぎるボイドを押し返す。
						// separationDistance を超えたボイドからは反発力を受けない
						//----------------------------------------------------------
						if (_distance < _params.separationDistance)
						{
							const Math::Vector3 _direction = _offset / _distance;

							// 近いほど1に近づき、separationDistance で0になる。
							// 少し近いなら弱く、極端に近ければ強く押し返す
							const float _ratio = (_params.separationDistance - _distance) / _params.separationDistance;
							_separation += _direction * (_ratio * _ratio);
						}

						//----------------------------------------------------------
						// Alignment / Cohesion : 周囲の個体と軍隊として行動する。
						// 近すぎる個体を押し返す Separation とは別の範囲を使う
						//----------------------------------------------------------
						if (_distance < _params.neighborRadius)
						{
							_alignment += _other.velocity;
							_cohesion += _other.position;
							++_neighborCount;
						}
					}

					// Alignment / Cohesion を平均化
					if (_neighborCount > 0)
					{
						const float _neighborCountInv = 1.0f / static_cast<float>(_neighborCount);

						// Alignment : 周囲の平均速度との差
						_alignment *= _neighborCountInv;
						_alignment -= _velComp.value;

						// Cohesion : 周囲の平均位置へ向かう方向
						_cohesion *= _neighborCountInv;
						_cohesion -= _trsComp.pos;
					}

					// Steering を合成
					Math::Vector3 _steering = {};
					_steering += _separation * _params.separationWeight;
					_steering += _alignment * _params.alignmentWeight;
					_steering += _cohesion * _params.cohesionWeight;

					//--------------------------------------------------------------
					// Seek : 目標地点へ向かうべき速度と現在速度との差から作る
					//--------------------------------------------------------------
					Math::Vector3 _toTarget = a_targetArray[_i].targetPos - _trsComp.pos;
					const float _targetDistance = _toTarget.Length();
					if (_targetDistance > 0.001f)
					{
						_toTarget /= _targetDistance;

						// 目標地点付近では減速する
						float _targetSpeed = _params.maxSpeed;
						if (_targetDistance < _params.slowRadius)
						{
							_targetSpeed *= _targetDistance / _params.slowRadius;
						}

						const Math::Vector3 _seek = _toTarget * _targetSpeed - _velComp.value;
						_steering += _seek * _params.seekWeight;
					}

					// Steering の最大値を制限する(分離・結合・Seek で吹き飛ぶのを防ぐ)
					if (_steering.LengthSquared() > _params.maxSteeringForce * _params.maxSteeringForce)
					{
						_steering.Normalize();
						_steering *= _params.maxSteeringForce;
					}

					// 速度の更新
					_velComp.value += _steering * a_ctx.dt;

					// 最大速度制限 : Seek 以外の力でも速度は上がるので、最終的な速度にも掛ける
					if (_velComp.value.LengthSquared() > _params.maxSpeed * _params.maxSpeed)
					{
						_velComp.value.Normalize();
						_velComp.value *= _params.maxSpeed;
					}
				}
			}
		)
		// 順序 : 全員の更新前の値(写し)を作ってから操舵する。
		// 写しは速度を読み、こちらは速度を書くので、読み書きだけでは循環する
		.After("BoidSnapshotSystem")
		.ReadsResource<InstanceResource::BoidSnapshotResource>();
	}
}
