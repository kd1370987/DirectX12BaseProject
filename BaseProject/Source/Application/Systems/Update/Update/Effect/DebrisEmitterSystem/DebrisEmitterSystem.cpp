#include "DebrisEmitterSystem.h"

#include <cstring>

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Effect/DebrisEmitterComponent.h"
#include "Application/Components/Effect/BallisticComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Utility/PrefabSpawnHelper.h"
#include "Application/Utility/EffectPrefabSpawnHelper.h"

//==============================================================================
// DebrisEmitterSystem
//
// DebrisEmitterComponent を持つもの(砂柱の演出のルートなど)が生まれたら、
// 破片の EffectPrefab をまとめて撒く。撒くのは1度だけ。
//
//   向き = 水平の向き(一周の中から乱数) × cos(仰角) + 真上 × sin(仰角)
//   位置 = 中心 + 水平の向き × startRadius + 真上 × startHeight
//
// ・初速・回転・大きさは破片のルートへ生成前に書き込む(BallisticComponent / LocalTransform)。
//   飛ばすのは BallisticSystem。
// ・破片も EffectPrefab なので、SpawnEffectPrefab を通れば時間で必ず消える。
// ・破片のハンドルは PostDeserialize で取る(DebrisEmitterFixupSystem)。
//   プレハブから写した値は持ち主ではないので、必ず取り直す。
//==============================================================================
namespace
{
	// 向きがそろわない回転軸(単位ベクトル)
	Math::Vector3 RandomAxis()
	{
		Math::Vector3 _axis(
			Math::Random::Float(-1.0f, 1.0f),
			Math::Random::Float(-1.0f, 1.0f),
			Math::Random::Float(-1.0f, 1.0f));
		if (_axis.LengthSquared() < 1e-6f) _axis = Math::Vector3(0.0f, 1.0f, 0.0f);
		_axis.Normalize();
		return _axis;
	}

	// 下限と上限が逆に入っていても引けるようにする
	float RandomRange(float a_min, float a_max)
	{
		return Math::Random::Float((std::min)(a_min, a_max), (std::max)(a_min, a_max));
	}
}

void DebrisEmitterSystem::Init(App::ECS::APPWorld& a_world)
{
	//--------------------------------------------------------------------------
	// 破片のハンドルを取る
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<DebrisEmitterComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"DebrisEmitterFixupSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag*               a_tag,
			DebrisEmitterComponent*           a_emitterArray
		)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				DebrisEmitterComponent& _emitter = a_emitterArray[_i];
				_emitter.isEmitted = false;

				if (_emitter.debrisGUID == Engine::DefaultGUID)
				{
					_emitter.debrisHandle = {};
					continue;
				}

				// 撒く瞬間に読み込みが走らないよう、ここで読み切っておく
				_pResourceManager->AcquireImmediate(_emitter.debrisHandle, _emitter.debrisGUID);
			}
		}
	);

	//--------------------------------------------------------------------------
	// 撒く
	//--------------------------------------------------------------------------
	a_world.ActiveTask<DebrisEmitterComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"DebrisEmitterSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			DebrisEmitterComponent*           a_emitterArray,
			const LocalTransformComponent*    a_transArray
		)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			auto& _world = *a_ctx.pWorld;
			const auto _ballisticTypeID = _world.GetCompTypeID<BallisticComponent>();
			const auto _transTypeID     = _world.GetCompTypeID<LocalTransformComponent>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				DebrisEmitterComponent& _emitter = a_emitterArray[_i];
				if (_emitter.isEmitted) continue;
				_emitter.isEmitted = true;

				const auto* _pDebris = _pResourceManager->Get(_emitter.debrisHandle);
				if (!_pDebris) continue;

				const Math::Vector3 _center = a_transArray[_i].pos;

				for (int _n = 0; _n < _emitter.count; ++_n)
				{
					const float _azimuth   = Math::Random::Float(0.0f, DirectX::XM_2PI);
					const float _elevation = DirectX::XMConvertToRadians(
						RandomRange(_emitter.elevationMinDeg, _emitter.elevationMaxDeg));
					const float _speed     = RandomRange(_emitter.speedMin, _emitter.speedMax);

					const Math::Vector3 _horizontal(std::cos(_azimuth), 0.0f, std::sin(_azimuth));
					const Math::Vector3 _dir =
						_horizontal * std::cos(_elevation) + Math::Vector3::Up() * std::sin(_elevation);

					const Math::Vector3 _pos =
						_center + _horizontal * _emitter.startRadius + Math::Vector3::Up() * _emitter.startHeight;

					const Math::Vector3 _spinAxis = RandomAxis();
					const float _spinSpeed = RandomRange(_emitter.spinMinDeg, _emitter.spinMaxDeg);
					const float _scale     = RandomRange(_emitter.scaleMin, _emitter.scaleMax);

					// 見た目がそろわないよう、向きも最初からばらしておく
					const Math::Quaternion _rot = Math::Quaternion::CreateFromAxisAngle(
						RandomAxis(), Math::Random::Float(0.0f, DirectX::XM_2PI));

					App::Utility::SpawnEffectPrefab(_world, *_pDebris, _pos,
						[&](Engine::ECS::World& a_w, std::vector<Engine::Resource::PrefabInstanceData>& a_instanceVec)
						{
							auto& _root = a_instanceVec[0];

							// 初速と回転(飛ばすのは BallisticSystem)
							if (uint8_t* _pBuf = App::Utility::EnsureInstanceComponent(a_w, _root, _ballisticTypeID))
							{
								BallisticComponent _ballistic = {};
								std::memcpy(&_ballistic, _pBuf, sizeof(_ballistic));
								_ballistic.velocity     = _dir * _speed;
								_ballistic.spinAxis     = _spinAxis;
								_ballistic.spinSpeedDeg = _spinSpeed;
								std::memcpy(_pBuf, &_ballistic, sizeof(_ballistic));
							}

							// 大きさと向き(プレハブの保存値に掛ける)
							if (uint8_t* _pBuf = App::Utility::EnsureInstanceComponent(a_w, _root, _transTypeID))
							{
								LocalTransformComponent _trans = {};
								std::memcpy(&_trans, _pBuf, sizeof(_trans));
								_trans.scale  *= _scale;
								_trans.quat    = _rot * _trans.quat;
								_trans.isDirty = true;
								std::memcpy(_pBuf, &_trans, sizeof(_trans));
							}
						}
					);
				}
			}
		}
	);
}
