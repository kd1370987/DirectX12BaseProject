#include "ChargeDashSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Force/MovementComponent.h"
#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Character/LookAngleComponent.h"
#include "../../../../../Components/Character/Robot/ChargeDashComponent.h"

//==============================================================================
// ChargeDashSystem
//
// ジャンプを溜めて撃ち出すチャージダッシュの進行。
// 溜め → 発動 → 直進 → 停止 の一本道を、ChargeDashComponent の上で回す。
//
// ・向きは発動した瞬間に1回だけ決めて dashDir へ焼き付ける。
//   ブースト(RobotBoostSystem)は毎フレーム入力から向きを作り直すので曲がれるが、
//   こちらは焼き付けた向きしか使わないので曲がれない。
//   視点を回しても軌道は変わらず、変わるのは「逆入力で止まるかどうか」の判定だけ。
//
// ・速度の入れ方
//   VelocityComponent(目標速度)だけを書くと、実速度は MovementIntegrationSystem が
//   MovementComponent.acceleration で追いかける。プレイヤーの加速度は 150 前後なので
//   dashSpeed 90 に乗るまで 0.6 秒ほどかかり、dashTime とほぼ同じ長さになってしまう。
//   撃ち出しの手応えが出ないので、実速度(MovementComponent.velocity)にも同じ値を直接入れて
//   その場で最高速へ乗せる。目標と実速度が一致していれば、
//   後段の MovementIntegrationSystem は差分 0 でそのまま通す。
//
//   MovementComponent をクエリに入れずに RefData で引くのは依存が輪になるため。
//   クエリに入れると
//     「こちらが Velocity を書く → MovementIntegration が読む」
//     「MovementIntegration が Movement を書く → こちらが読む」
//   の2辺で循環し、システムの順序が決められなくなる。
//
// ・実行帯は Physics。速度を書く仲間(GravitySystem / RobotBoostSystem)より後に
//   登録して、ダッシュ中はこちらの値が最後に残るようにしている
//   (書き手同士の順序は登録順で決まる。読み手の MovementIntegrationSystem とは
//    Velocity の読み書きで辺が張られるので、登録位置に関わらず後ろへ回る)。
//==============================================================================
void ChargeDashSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<ChargeDashComponent, VelocityComponent, const MoveIntentComponent,
		const LookAngleComponent>(
		Engine::ECS::ESystemType::Physics,
		"ChargeDashSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			ChargeDashComponent* a_dashArray,
			VelocityComponent* a_velArray,
			const MoveIntentComponent* a_moveIntentArray,
			const LookAngleComponent* a_lookArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ChargeDashComponent&       _dashComp   = a_dashArray[_i];
				VelocityComponent&         _velComp    = a_velArray[_i];
				const MoveIntentComponent& _moveIntent = a_moveIntentArray[_i];
				const LookAngleComponent&  _lookComp   = a_lookArray[_i];

				// 出た瞬間の印は1フレームだけ立てたいので、毎フレーム倒しておく
				_dashComp.isJustDashed = false;

				//----------------------------------------------------------
				// 入力の向きを水平のワールド方向へ直す
				//
				// 発動時の進行方向と、逆入力で止める判定の両方で使う。
				// 水平前方 = (sinYaw, 0, cosYaw) / 右 = (cosYaw, 0, -sinYaw) は
				// RobotBoostSystem・CharacterMovementSystem と同じ組み立て
				//----------------------------------------------------------
				const float _yawRad = DirectX::XMConvertToRadians(_lookComp.Yaw);
				const float _sinY = std::sin(_yawRad);
				const float _cosY = std::cos(_yawRad);

				const Math::Vector3 _forward(_sinY, 0.0f, _cosY);

				Math::Vector3 _inputDir =
					_forward * _moveIntent.value.z +
					Math::Vector3(_cosY, 0.0f, -_sinY) * _moveIntent.value.x;

				const bool _hasInput = (_inputDir.LengthSquared() > 1e-6f);
				if (_hasInput)
				{
					_inputDir.Normalize();
				}

				//----------------------------------------------------------
				// 実速度への直接書き込み(加速度を飛ばして最高速へ乗せる)
				//
				// MovementComponent を持たない相手は目標速度だけで動くので、
				// 持っているときだけ書く。
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				//----------------------------------------------------------
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				MovementComponent* _pMovement = nullptr;
				if (a_ctx.pWorld->HasComponent<MovementComponent>(_self))
				{
					_pMovement = a_ctx.pWorld->RefData<MovementComponent>(_self);
				}

				// 目標速度と実速度へ同じ水平速度を入れる
				auto _applyHorizontal = [&](const Math::Vector3& a_dir, float a_speed)
				{
					_velComp.value.x = a_dir.x * a_speed;
					_velComp.value.z = a_dir.z * a_speed;

					if (_pMovement)
					{
						_pMovement->velocity.x = _velComp.value.x;
						_pMovement->velocity.z = _velComp.value.z;
					}
				};

				// 溜めを最初から積み直す
				auto _resetCharge = [&]()
				{
					_dashComp.chargeTimer = 0.0f;
					_dashComp.charge01    = 0.0f;
					_dashComp.isCharging  = false;
					_dashComp.isCharged   = false;
				};

				// クールタイムを進める
				if (_dashComp.coolTimer > 0.0f)
				{
					_dashComp.coolTimer = (std::max)(0.0f, _dashComp.coolTimer - a_ctx.dt);
				}

				//----------------------------------------------------------
				// ダッシュ中
				//----------------------------------------------------------
				if (_dashComp.isDashing)
				{
					_dashComp.dashTimer = (std::max)(0.0f, _dashComp.dashTimer - a_ctx.dt);

					// 進行方向の逆へ入力されたか(既定の操作なら S)。
					// キーではなくワールドでの向きで見ているので、
					// 視点を回して「進んでいる向きの逆」を入れても止まる
					const bool _isBrake =
						_hasInput && (_inputDir.Dot(_dashComp.dashDir) <= _dashComp.brakeDot);

					const bool _isTimeUp = (_dashComp.dashTimer <= 0.0f);

					if (_isBrake || _isTimeUp)
					{
						// 抜けるときは 0 にせず少しだけ速度を残す。
						// いきなり止めると、そのフレームだけ画も操作も固まって見える
						const float _exitSpeed =
							_dashComp.dashSpeed * std::clamp(_dashComp.exitSpeedScale, 0.0f, 1.0f);

						_applyHorizontal(_dashComp.dashDir, _exitSpeed);

						_dashComp.isDashing = false;
						_dashComp.dashTimer = 0.0f;
						_dashComp.coolTimer = _dashComp.coolTime;
					}
					else
					{
						_applyHorizontal(_dashComp.dashDir, _dashComp.dashSpeed);

						// 高さを保つ : 直前に GravitySystem が足した落下ぶんと、
						// 上下入力から CharacterMovementSystem が入れた上昇ぶんを打ち消す
						if (_dashComp.isKeepHeight)
						{
							_velComp.value.y = 0.0f;
						}
					}

					// 出ている間は溜め直せない(押しっぱなしで出た直後に溜まり始めないように)
					_resetCharge();
					continue;
				}

				//----------------------------------------------------------
				// 溜め
				//
				// クールタイム中は溜め始められない。押しっぱなしのまま待たれると
				// 明けた瞬間に溜まってしまうので、明けてから積み始める
				//----------------------------------------------------------
				const bool _canCharge = (_dashComp.coolTimer <= 0.0f);

				if (_dashComp.isChargeIntent && _canCharge)
				{
					_dashComp.chargeTimer += a_ctx.dt;
					if (_dashComp.chargeTime > 0.0f)
					{
						// 溜まりきってからも押し続けられるので、上限で止めておく
						_dashComp.chargeTimer = (std::min)(_dashComp.chargeTimer, _dashComp.chargeTime);
					}
					_dashComp.isCharging = true;
				}
				else
				{
					_dashComp.isCharging = false;
				}

				// 溜まり具合。chargeTime が 0 なら押した瞬間から溜まりきっている扱い
				_dashComp.charge01 = (_dashComp.chargeTime > 0.0f)
					? std::clamp(_dashComp.chargeTimer / _dashComp.chargeTime, 0.0f, 1.0f)
					: ((_dashComp.isCharging || _dashComp.isChargeRelease) ? 1.0f : 0.0f);

				// 溜まりきったかは積んだ時間だけで見る。
				// 離したフレームは isCharging が倒れているので、
				// 「押しているか」を混ぜると離した瞬間に発動できなくなる
				_dashComp.isCharged = (_dashComp.charge01 >= 1.0f);

				//----------------------------------------------------------
				// 発動するか
				//
				// 既定は「溜まりきってから離した瞬間」。
				// isAutoRelease なら溜まりきったフレームでそのまま出る
				//----------------------------------------------------------
				bool _isFire = false;
				if (_dashComp.isCharged)
				{
					_isFire = _dashComp.isAutoRelease || _dashComp.isChargeRelease;
				}

				if (!_isFire)
				{
					// 溜まりきる前に離したら積み直し。
					// 押していない間もここを通るので、溜めは押し続けたぶんだけになる
					if (!_dashComp.isChargeIntent)
					{
						_resetCharge();
					}
					continue;
				}

				//----------------------------------------------------------
				// 発動
				//
				// 向きはここで1回だけ決めて焼き付ける。以降は入力を見ない
				//----------------------------------------------------------
				Math::Vector3 _dir = (_dashComp.isUseMoveDir && _hasInput) ? _inputDir : _forward;
				_dir.y = 0.0f;

				const float _lenSq = _dir.LengthSquared();
				if (_lenSq > 1e-6f)
				{
					_dir /= std::sqrt(_lenSq);
				}
				else
				{
					// 視点がほぼ真上/真下でも水平の向きは作れるが、念のための保険
					_dir = { 0.0f, 0.0f, 1.0f };
				}

				_dashComp.dashDir      = _dir;
				_dashComp.isDashing    = true;
				_dashComp.isJustDashed = true;
				_dashComp.dashTimer    = _dashComp.dashTime;

				_resetCharge();

				// 出たフレームからいきなり最高速で進む
				_applyHorizontal(_dir, _dashComp.dashSpeed);
				if (_dashComp.isKeepHeight)
				{
					_velComp.value.y = 0.0f;
				}
			}
		}
	);
}
