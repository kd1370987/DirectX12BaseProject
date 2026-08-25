#include "AimTargetSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSCameraStateComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Character/AimTargetPosComponent.h"
#include "../../../../Components/Character/LockOnTargetComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

//==========================================================================================
// AimTargetSystem
//
// アクティブカメラの正面へレイを撃ち、その着弾点を
// 「カメラがフォーカスしている対象」の AimTargetPosComponent へ書き込む。
// 銃はこの狙点へ向けて弾を飛ばす(GunShootSystem)。
//
// ・Camera フェーズに置く理由
//     TPSSystem がカメラ姿勢を確定させた後に撃たないと、1フレーム前の向きで狙うことになる。
//     また動的TLASは Update 後(Physics の前)に構築済みなので、
//     このフェーズなら静的地形も動く敵も両方拾える。
//   → 狙点が銃に届くのは次フレームの PreUpdate(AttachmentDispatchSystem)。
//
// ・書き込み先はフォーカス対象エンティティなので、クエリ外への RefData 書き込みになる。
//   AttachmentDispatchSystem と同じやり方で、同フェーズに読み手がいないことが前提。
//
// ・撃つのはメインカメラの1台だけ。どれかは MainCameraSystem が決めて
//   SingletonEntityResource.mainCamera に置いてあるので、ここでは引くだけにする。
//   カスタムタスクにしているのは、クエリを回さず1体だけ引く形になったため。
//   ReadList に LocalTransformComponent を挙げているのは
//   「カメラの姿勢を書く TPSSystem より後ろに並ぶ」ための依存で、
//   これが無いと1フレーム前の向きで狙うことになる。
//==========================================================================================
void AimTargetSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::Camera,
		Engine::ECS::ReadList<CameraTag, FollowTargetComponent, LocalTransformComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<SingletonEntityResource>()) return;

			const auto& _singleton = a_ctx.pWorld->GetResource<SingletonEntityResource>();

			const Engine::ECS::Entity _self = _singleton.mainCamera;
			if (_self == Engine::ECS::Limits::INVALID_ENTITY) return;
			if (!a_ctx.pWorld->IsAliveEntity(_self)) return;

			// 追従先を持たないカメラ(固定カメラなど)は狙点を作らない
			if (!a_ctx.pWorld->HasComponent<FollowTargetComponent>(_self))		return;
			if (!a_ctx.pWorld->HasComponent<LocalTransformComponent>(_self))	return;

			const auto* _pFollow	= a_ctx.pWorld->RefData<FollowTargetComponent>(_self);
			const auto* _pTrs		= a_ctx.pWorld->RefData<LocalTransformComponent>(_self);
			if (!_pFollow || !_pTrs) return;

			auto* _pCollWorld = &a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();

			const FollowTargetComponent&	_followComp = *_pFollow;
			const LocalTransformComponent&	_trsComp	= *_pTrs;

			//============================================================
			// フォーカス対象の狙点コンポーネントを取得
			//============================================================
			Engine::ECS::Entity _target = _followComp.target;
			if (_target == Engine::ECS::Limits::INVALID_ENTITY) return;
			if (!a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_target)) return;

			AimTargetPosComponent* _pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_target);
			if (!_pAim) return;

			//============================================================
			// カメラ正面
			//------------------------------------------------------------
			// このエンジンは左手系で、ローカル +Z が前方(WorldMatrix の _31.._33 と同じ軸)。
			// SimpleMath の Vector3::Forward は (0,0,-1) で逆を向くため使わない。
			//============================================================
			Math::Vector3 _fwd = Math::Vector3::Transform(
				Math::Vector3(0.0f, 0.0f, 1.0f),
				Math::Quaternion(_trsComp.quat)
			);

			// 長さのチェックは NaN も弾ける形で書くこと。
			// (NaN は比較が常に false になるので「_lenSq < 閾値 なら continue」だけだと素通りし、
			//  正規化しても NaN のまま Raycast へ流れて XMVector3IsUnit のアサートで落ちる)
			float _fwdLenSq = _fwd.LengthSquared();
			if (!(_fwdLenSq > 1e-8f)) return;
			_fwd /= std::sqrt(_fwdLenSq);

			//============================================================
			// レイの始点
			//------------------------------------------------------------
			// カメラ位置から撃つと、カメラと自機の間にある自機の武器やブースターを拾う。
			// これらは自機とは別エンティティなので Raycast の myID では除外できず、
			// 狙点が自機の手前(＝銃口より後ろ)に来て、弾がカメラへ向かって飛んでしまう。
			//
			// そこで「カメラが実際に見ている点(フォーカス点)」をカメラ前方軸へ射影し、
			// そこから startOffset だけ先を始点にする。
			// これならカメラ距離を変えても自動で追従する。
			//============================================================
			// フォーカス点は TPSSystem が解決済みの値をそのまま使う。
			// offsetPos はカメラ空間(オービット基準)なので、ここで
			// ワールド座標へ組み直すとオービットの式を二重に持つことになり、
			// 片方だけ直したときに静かにズレる。
			//
			// TPSSystem はカメラの LocalTransform を書き、こちらはそれを読むので
			// RAW 依存でこのシステムが後になる = 同フレームの値が入っている。
			// TPS 以外のカメラなど、状態が無い場合はフォーカス点を諦めて
			// startOffset だけで撃つ(従来通り)。
			float _startDist = _pAim->startOffset;
			if (a_ctx.pWorld->HasComponent<TPSCameraStateComponent>(_self))
			{
				if (const auto* _pState = a_ctx.pWorld->RefData<TPSCameraStateComponent>(_self))
				{
					if (_pState->isInitialized)
					{
						// カメラ前方軸への射影 = カメラからフォーカス点までの前方距離
						float _proj =
							(Math::Vector3(_pState->lookAtWorld) - Math::Vector3(_trsComp.pos)).Dot(_fwd);
						_startDist = std::max(_proj, 0.0f) + _pAim->startOffset;
					}
				}
			}

			//============================================================
			// レイ発射
			//------------------------------------------------------------
			// フォーカス対象自身は myID で除外する。
			//============================================================
			Engine::Collision::RayInfo _info;
			_info.origin		= Math::Vector3(_trsComp.pos) + _fwd * _startDist;
			_info.direction		= _fwd;
			_info.maxDistance	= _pAim->maxDistance;

			Engine::Collision::Result _res = {};
			bool _isHit = _pCollWorld->Raycast(_info, _res, _target);

			//============================================================
			// 狙点の確定
			//------------------------------------------------------------
			// 当たらなかった場合も「最大距離の点」を入れておく。
			// こうしておけば銃側は常に狙点へ撃つだけでよい(空撃ちの分岐が要らない)。
			//============================================================
			if (_isHit)
			{
				_pAim->pos			= _res.hitPos;
				_pAim->hitEntity	= _res.hitEntity;
			}
			else
			{
				_pAim->pos			= Math::Vector3(_info.origin) + _fwd * _info.maxDistance;
				_pAim->hitEntity	= Engine::ECS::Limits::INVALID_ENTITY;
			}
			_pAim->dir		= _fwd;		// 狙いの向き。銃側で基準軸として使う
			_pAim->isHit	= _isHit;
			_pAim->isValid	= true;

			//============================================================
			// ロック中はロック相手を狙点にする
			//------------------------------------------------------------
			// レイはカメラ前方＝レティクルの下を狙うので、
			// 「レティクルには入っているが枠の中心からは少しずれている」相手には
			// 当たらない。さらにカメラの前方は構図のオフセット
			// (CameraFocusTargetComponent.offsetPos.x)ぶん横へ振れているため、
			// 空振りしたときの狙点は自機から見て常に同じ側へ寄る。
			// 体はロック相手を向く(LockOnRotationSystem)ので、狙点だけ
			// カメラ前方のままだと見た目と弾道がずれてしまう。
			//
			// ロック結果は PostUpdate(LockOnTargetSystem)が書くので1フレーム前だが、
			// 狙点はもともと次フレームに銃へ配られるので差は出ない。
			//============================================================
			bool _isLockAim = false;
			if (a_ctx.pWorld->HasComponent<LockOnTargetComponent>(_target))
			{
				if (const auto* _pLockOn = a_ctx.pWorld->RefData<LockOnTargetComponent>(_target))
				{
					if (_pLockOn->IsLocked())
					{
						Math::Vector3 _toLock =
							Math::Vector3(_pLockOn->lockedPos) - Math::Vector3(_info.origin);

						// 長さのチェックは NaN も弾ける形で書くこと
						float _toLockLenSq = _toLock.LengthSquared();
						if (_toLockLenSq > 1e-6f)
						{
							_pAim->pos       = _pLockOn->lockedPos;
							_pAim->dir       = _toLock / std::sqrt(_toLockLenSq);
							_pAim->hitEntity = _pLockOn->lockedEntity;
							_pAim->isHit     = true;
							_isLockAim       = true;
						}
					}
				}
			}

			// デバッグ表示(赤=ロック狙点, 青=ヒット, 白=空振り)
			if (_isLockAim)
			{
				a_ctx.pServices->pMainEditor->DrawLine(
					_info.origin, Math::Vector3(_pAim->pos), Engine::Color::RED);
			}
			else
			{
				a_ctx.pServices->pMainEditor->DrawRay(
					_info.origin, _info.direction, _info.maxDistance, _isHit,
					_isHit ? Engine::Color::BLUE : Engine::Color::WHITE);
			}
		}
	);
}
