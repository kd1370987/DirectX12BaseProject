#include "BossMissileSalvoSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boss/BossParamsComponent.h"
#include "Application/Components/Combat/TargetEntityComponent.h"
#include "Application/Components/Attachment/AttachmentSlotsComponent.h"
#include "Application/Components/Weapon/MissileLockComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Systems/Weapon/MissileSalvo.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Physics/Collider.h"
#include "Application/Components/Transform/HierarchyComponent.h"
#include "Application/Components/Enemy/EnemyTag.h"
#include "Application/Components/Weapon/GunStateComponent.h"

namespace App::System
{
	//==========================================================================================
	// BossMissileSalvoSystem
	//
	// ボスのミサイルを一斉射する。プレイヤー側(MissileSalvoSystem)との違いは溜め方だけ。
	//
	//   プレイヤー … ミサイルキーを押している間、コンバットレティクルの円に入った敵を
	//                 スクリーン座標で拾って溜め、離した瞬間に撃つ
	//   ボス       … 狙う相手は最初からプレイヤー1体に決まっているので溜めが要らない。
	//                 BossCombatIntentSystem が間隔で立てる要求(isMissileRequest)を受けて、
	//                 全弾をその1体へ割り振る
	//
	// 撃ち出し(ポッドの GunStateComponent から弾/弾速/銃口を引き、散らして生成する)は
	// まったく同じなので App::System::MissileSalvo::ConsumeFireQueue を共有する。
	// 弾数・間隔・散らし角・クールダウンの設定も MissileLockComponent をそのまま使うので、
	// ボス専用の設定項目は増えていない。
	//
	// ・PostUpdate に置く理由
	//     発射位置をミサイルポッドの「今フレームの」ワールド行列から取るため。
	//     WorldMatrixComponent を読むので CalcMatrixSystem /
	//     CommitHierarchyWorldMatrixSystem より自動的に後ろへ回る(プレイヤー側と同じ)。
	//==========================================================================================
	void BossMissileSalvoSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<
			Component::BossCommandComponent,
			Component::MissileLockComponent,
			const Component::AttachmentSlotsComponent,
			const Component::TargetEntityComponent,
			const Component::WorldMatrixComponent>(
			Engine::ECS::ESystemType::PostUpdate,
			"BossMissileSalvoSystem",
			[](
				Engine::ECS::Chunk*      /*a_pChunk*/,
				uint32_t                          a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag*                        /*a_tags*/,
				Component::BossCommandComponent*             a_commandArray,
				Component::MissileLockComponent*             a_missileArray,
				const Component::AttachmentSlotsComponent*   a_slotsArray,
				const Component::TargetEntityComponent*      a_targetArray,
				const Component::WorldMatrixComponent*       a_worldMatArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Component::BossCommandComponent&           _command   = a_commandArray[_i];
					Component::MissileLockComponent&           _missile   = a_missileArray[_i];
					const Component::AttachmentSlotsComponent& _slots     = a_slotsArray[_i];
					const Component::TargetEntityComponent&    _target    = a_targetArray[_i];
					const Component::WorldMatrixComponent&     _selfWorld = a_worldMatArray[_i];

					// クールダウンを進める
					if (_missile.cooldownTimer > 0.0f)
					{
						_missile.cooldownTimer = std::max(_missile.cooldownTimer - a_ctx.dt, 0.0f);
					}

					const Math::Matrix& _selfMat = _selfWorld.worldMat;
					const Math::Vector3 _selfPos = { _selfMat._41, _selfMat._42, _selfMat._43 };

					//==========================================================
					// 相手の位置
					//==========================================================
					const Engine::ECS::Entity _targetEntity = _target.targetEntity;

					bool          _hasTarget = false;
					Math::Vector3 _targetPos = {};

					if (_targetEntity != Engine::ECS::Limits::INVALID_ENTITY &&
						a_ctx.pWorld->HasComponent<Component::WorldMatrixComponent>(_targetEntity))
					{
						if (const auto* _pTargetWorld =
							a_ctx.pWorld->RefData<Component::WorldMatrixComponent>(_targetEntity))
						{
							_targetPos = Math::Matrix(_pTargetWorld->worldMat).Translation();
							_targetPos.y += _missile.targetOffsetY;
							_hasTarget = true;
						}
					}

					//==========================================================
					// 発射キューを作る
					//----------------------------------------------------------
					// 要求は成否に関わらず1回で消す。撃てない状況(発射中/クールダウン中)
					// のぶんを溜めておくと、明けた瞬間に立て続けに撃ってしまうため。
					// 次の間隔でまた要求が立つので取りこぼしにはならない。
					//==========================================================
					if (_command.isMissileRequest)
					{
						_command.isMissileRequest = false;

						const int _fireCount = _missile.GetActiveMissileCount();
						const bool _canFire =
							!_missile.IsFiring() && !_missile.IsCoolDown() &&
							(_fireCount > 0) && _hasTarget;

						if (_canFire)
						{
							// 相手は1体なので全弾が同じ的を追う。散らし角のぶんだけ
							// いったん広がってから、HomingSystem が寄せてくる
							for (int _m = 0; _m < _fireCount; ++_m)
							{
								_missile.fireTargets[_m] = _targetEntity;
							}

							_missile.fireTotal  = _fireCount;
							_missile.fireRemain = _fireCount;
							_missile.fireTimer  = 0.0f;

							// クールダウンは撃ち始めから数える
							_missile.cooldownTimer = std::max(_missile.cooldown, 0.0f);
						}
					}

					//==========================================================
					// 発射キューの消化(プレイヤーと共通)
					//==========================================================
					if (!_missile.IsFiring()) continue;

					// 相手が消えた弾のための基準の向き。相手が居るならあちらが上書きする
					Math::Vector3 _aimDir = _hasTarget
						? (_targetPos - _selfPos)
						: Math::Vector3(_selfMat._31, _selfMat._32, _selfMat._33);	// 左手系 +Z = 前方

					App::System::MissileSalvo::ConsumeFireQueue(
						a_ctx, _missile, _slots.missile.id, _aimDir);
				}
			}
		)
		// 順序 : ポッドの銃設定(GunState)とミサイルの溜めの書き手同士(プレイヤーとボスで対象は重ならない)
		.After("MissileSalvoSystem")
		// 絞り込みに使わない読み書き : ポッドの銃設定(弾のハンドルを解決して書く)と発射元、狙う相手の位置
		.Reads<Component::ModelComponent, Component::ColliderComponent, Component::HierarchyComponent, Component::EnemyTag, Component::WorldMatrixComponent>()
		.Writes<Component::GunStateComponent>();
	}
}
