#include "BoostSoundSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../Components/Character/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../Components/Resource/AudioBehaviorComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../../Engine/Audio/AudioManager.h"

//==========================================================================================
// BoostSoundSystem
//
// ブースト状態から音を鳴らす。ThrusterEffectSystem のサウンド版で、
// 噴射エフェクトと同じスロット構成をそのまま使う。
//
// 鳴らす中身は AudioBehavior アセットが持っているので、ここが伝えるのは状態だけ。
//
//   噴射に入った瞬間 -> Start + Loop
//   噴射している間   -> Loop (毎フレーム呼んでよい)
//   噴射が終わった後 -> End  (始動していたときだけ1回鳴る)
//
// 起動・終了の判定は「推力が出ているか」の立ち上がり/立ち下がりだけで決める。
// 入力の押下フラグを見ないので、プレイヤーでもボスでも、
// 燃料切れで落ちた場合でも同じように鳴る。
//
// 始動音だけ・継続音だけといった組み合わせも、アセット側の空欄で表現できる。
//
// 対象はブースターを付けている親自身と、スロットが指すブースター子エンティティ。
// どれも同じ流れを受け取るので、機体側で1つ鳴らす・ノズルごとに鳴らすの
// どちらもアセットの割り当てだけで決められる。
//
// 子エンティティと親自身の AudioBehaviorComponent はこのクエリに含まれないため
// World::RefData で横断参照する(構造変更は行わないので反復中でも安全)。
//==========================================================================================
void BoostSoundSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const BoostComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"BoostSoundSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const BoostComponent* a_boostArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pAudioManager || !_pResourceManager) return;

			// エンティティ1体へブーストの状態を伝える。
			// ビヘイビアが付いていない・まだ読めていない相手は黙って飛ばす
			auto _driveBehavior =
				[&a_ctx, _pAudioManager, _pResourceManager]
				(Engine::ECS::Entity a_entity, bool a_isBoosting)
			{
				if (a_entity == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_ctx.pWorld->HasComponent<AudioBehaviorComponent>(a_entity)) return;

				auto* _pComp = a_ctx.pWorld->RefData<AudioBehaviorComponent>(a_entity);
				if (!_pComp) return;

				auto* _pBehavior = _pResourceManager->Ref(_pComp->behaviorHandle);
				if (!_pBehavior) return;

				// 3D指定のパートを鳴らす位置。
				// 鳴らす前に入れておかないと、原点で鳴ってから移動することになる。
				// (2D指定しか入っていないビヘイビアでは読み捨てられる)
				//
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				if (a_ctx.pWorld->HasComponent<WorldMatrixComponent>(a_entity))
				{
					if (auto* _pWorldComp = a_ctx.pWorld->RefData<WorldMatrixComponent>(a_entity))
					{
						_pComp->instance.SetPos(*_pAudioManager, Math::Matrix(_pWorldComp->worldMat).Translation());
					}
				}

				if (a_isBoosting)
				{
					// 噴射に入った最初のフレームだけ始動音を鳴らす。
					//
					// 入力側の「押した瞬間」フラグ(isBoostTriger)は見ない。
					// あれはプレイヤー入力でしか立たず、ボスのように
					// isBoostIntent だけ立てて噴射に入る相手だと始動音が抜ける。
					// ビヘイビア自身が「始動〜終了の間か」を isActive で覚えているので、
					// その立ち上がりを見れば誰が噴射させたかによらず1回だけ鳴らせる
					if (!_pComp->instance.isActive)
					{
						_pBehavior->Start(*_pAudioManager, _pComp->instance);
					}

					// 始動音と噴射音は同じフレームから重なって鳴る
					_pBehavior->Loop(*_pAudioManager, _pComp->instance);
				}
				else
				{
					// 始動していたときだけ終了音が鳴る(End側で見ている)
					_pBehavior->End(*_pAudioManager, _pComp->instance);
				}
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const AttachmentSlotsComponent& _slots = a_slotsArray[_i];
				const BoostComponent& _boost = a_boostArray[_i];

				// 実際に推力が出る条件。
				// 燃料切れで飛べないときに音だけ鳴らないよう、
				// RobotBoostSystem / ThrusterEffectSystem と同じ判定にしている。
				// 燃料切れで落ちたときも、そのまま終了音まで流れる
				const bool _hasFuel  = _boost.currentFuel > _boost.boostFuel;
				const bool _boosting = _boost.isBoostIntent && _hasFuel;

				// ブースターを付けている親自身
				_driveBehavior(a_pChunk->entityData[_i], _boosting);

				// スロットが指すブースター側
				_driveBehavior(_slots.rightShoulderBoost.id, _boosting);
				_driveBehavior(_slots.leftShoulderBoost.id, _boosting);
				_driveBehavior(_slots.rightLegBoost.id, _boosting);
				_driveBehavior(_slots.leftLegBoost.id, _boosting);
			}
		}
	);
}
