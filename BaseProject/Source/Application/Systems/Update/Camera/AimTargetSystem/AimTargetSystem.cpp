#include "AimTargetSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/CameraFocusTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Character/AimTargetPosComponent.h"

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
//==========================================================================================
void AimTargetSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActiveCameraTag, const CameraTag, const FollowTargetComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Camera,
		"AimTargetSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ActiveCameraTag* a_activeCamTagArray,
			const CameraTag* a_camTagArray,
			const FollowTargetComponent* a_followArray,
			const LocalTransformComponent* a_trsArray
			)
		{
			auto* _pCollWorld = a_ctx.pServices->pMainEngine->RefCollisionWorld();
			if (!_pCollWorld) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const FollowTargetComponent&	_followComp = a_followArray[_i];
				const LocalTransformComponent&	_trsComp	= a_trsArray[_i];

				//============================================================
				// フォーカス対象の狙点コンポーネントを取得
				//============================================================
				Engine::ECS::Entity _target = _followComp.target;
				if (_target == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_target)) continue;

				AimTargetPosComponent* _pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_target);
				if (!_pAim) continue;

				//============================================================
				// カメラ正面
				//------------------------------------------------------------
				// このエンジンは左手系で、ローカル +Z が前方(WorldMatrix の _31.._33 と同じ軸)。
				// SimpleMath の Vector3::Forward は (0,0,-1) で逆を向くため使わない。
				//============================================================
				DXSM::Vector3 _fwd = DXSM::Vector3::Transform(
					DXSM::Vector3(0.0f, 0.0f, 1.0f),
					DXSM::Quaternion(_trsComp.quat)
				);

				// 長さのチェックは NaN も弾ける形で書くこと。
				// (NaN は比較が常に false になるので「_lenSq < 閾値 なら continue」だけだと素通りし、
				//  正規化しても NaN のまま Raycast へ流れて XMVector3IsUnit のアサートで落ちる)
				float _fwdLenSq = _fwd.LengthSquared();
				if (!(_fwdLenSq > 1e-8f)) continue;
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
				float _startDist = _pAim->startOffset;
				if (a_ctx.pWorld->HasComponent<LocalTransformComponent>(_target))
				{
					if (const auto* _pTargetTRS = a_ctx.pWorld->RefData<LocalTransformComponent>(_target))
					{
						DXSM::Vector3 _focus = DXSM::Vector3(_pTargetTRS->pos);

						// フォーカスオフセット(TPSSystem の注視点と同じもの)
						if (a_ctx.pWorld->HasComponent<CameraFocusTargetComponent>(_target))
						{
							if (const auto* _pFocus = a_ctx.pWorld->RefData<CameraFocusTargetComponent>(_target))
							{
								_focus += DXSM::Vector3(_pFocus->offsetPos);
							}
						}

						// カメラ前方軸への射影 = カメラからフォーカス点までの前方距離
						float _proj = (_focus - DXSM::Vector3(_trsComp.pos)).Dot(_fwd);
						_startDist = std::max(_proj, 0.0f) + _pAim->startOffset;
					}
				}

				//============================================================
				// レイ発射
				//------------------------------------------------------------
				// フォーカス対象自身は myID で除外する。
				//============================================================
				Engine::Collision::RayInfo _info;
				_info.origin		= DXSM::Vector3(_trsComp.pos) + _fwd * _startDist;
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
					_pAim->pos			= DXSM::Vector3(_info.origin) + _fwd * _info.maxDistance;
					_pAim->hitEntity	= Engine::ECS::Limits::INVALID_ENTITY;
				}
				_pAim->dir		= _fwd;		// 狙いの向き。銃側で基準軸として使う
				_pAim->isHit	= _isHit;
				_pAim->isValid	= true;

				// デバッグ表示(青=ヒット, 白=空振り)
				a_ctx.pServices->pMainEditor->DrawRay(
					_info.origin, _info.direction, _info.maxDistance, _isHit,
					_isHit ? Engine::Color::BLUE : Engine::Color::WHITE);
			}
		}
	);
}
