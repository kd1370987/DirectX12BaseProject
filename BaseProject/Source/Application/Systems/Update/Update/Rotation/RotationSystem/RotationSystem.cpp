#include "RotationSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Character/LookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Tag/PlayerControllTag.h"

//==============================================================================
// RotationSystem
//
// LookAngleComponent から来た角度の通りに機体を向かせるだけの汎用システム。
// 「どこを向くべきか」を決めるのは別のシステムの仕事で、ここは適用専門。
//
// ・既定で使うのは Yaw だけ。Pitch は視線(TPSカメラ・上体の加算ポーズ)用の角度で、
//   これを機体へ適用すると体ごと前傾/後傾してしまうため、人型では使わない。
//   体ごと上下を向かせたいもの(空を泳ぐ群れのボスなど)は
//   LookAngleComponent::isApplyPitchToBody を立てると Pitch も入れて回す。
//   そのとき符号は反転させる(Pitch は上向きが正だが、X軸まわりの回転は
//   正で前方が下がる。AdditivePoseSystem が -Pitch を使っているのと同じ規約)。
// ・このエンジンは左手系でローカル +Z が前方。Vector3 オーバーロードは
//   (pitch,yaw,roll)順で軸が入れ替わるのでスカラー版を明示的に使う。
// ・プレイヤーは ActionState を見て挙動を切り替える LockOnRotationSystem が
//   姿勢を書くので、二重書き込みにならないよう PlayerControllTag を除外する。
//==============================================================================
void RotationSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const LookAngleComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"RotationSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const LookAngleComponent* a_lookArray,
			LocalTransformComponent* a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LookAngleComponent& _lookAng = a_lookArray[_i];
				LocalTransformComponent& _trs = a_trsArray[_i];

				// 角度は度で保持されているのでラジアンへ変換する
				const float _pitchDeg = _lookAng.isApplyPitchToBody ? -_lookAng.Pitch : 0.0f;

				Math::Quaternion _quat = Math::Quaternion::CreateFromYawPitchRoll(
					DirectX::XMConvertToRadians(_lookAng.Yaw),
					DirectX::XMConvertToRadians(_pitchDeg),
					0.0f
				);
				_quat.Normalize();

				_trs.quat = _quat;
				_trs.isDirty = true;	// 停止中でも行列を再構築させる
			}
		},
		Engine::ECS::Exclude<PlayerControllTag>{}
	);
}
