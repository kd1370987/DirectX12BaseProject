#include "RobotBoostSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Character/LookAngleComponent.h"
#include "../../../../../Components/Force/MovementComponent.h"

//==============================================================================
// RobotBoostSystem
//
// ブーストの推力を「進みたい向き」へ与える。
//
// ・向きは視点(LookAngle)の Yaw を基準に組み立てる。上下が付くのは次の2つの場合だけ。
//     (1) 上下の入力があるとき(MoveIntent.y ＝ ジャンプ / 急降下)
//         ワールドの上下をその分だけ足す。前後左右と混ぜれば斜め上下に飛ぶ。
//     (2) 入力がまったく無いとき
//         真上へ飛ぶ。視点の Pitch は見ないので、どこを向いていても上昇になる。
//   移動入力だけのとき(前後左右)は水平のまま。視点をどれだけ上下させても
//   地表を滑る挙動になり、歩いている方向とブーストで飛ぶ方向も一致する。
//
//   水平前方 = (sinYaw, 0, cosYaw)
//   右       = (cosYaw, 0, -sinYaw)
//   左手系 +Z 前方で、TPSSystem がカメラ姿勢に使っている
//   CreateFromYawPitchRoll(Yaw, -Pitch, 0) と同じ向き(Pitch が正で上向き)。
//   水平成分だけ見れば CharacterMovementSystem の式と一致する。
//
// ・速度そのものを差し替える(以前のように今の速度に倍率を掛けない)。
//   倍率方式だと止まっている時は 0 に何を掛けても 0 で飛べず、
//   ステートの都合で速度が潰されている時(canMove が false 等)も効かなかった。
//   boostPower は「ブースト時の速度(m/秒)」として扱う。
//
// ・Y を書くのも上記2つの場合だけ。そのときは直前に GravitySystem が足した
//   落下ぶんも打ち消されるので、吹かしている間は視線と上昇入力で高度を操れる。
//   水平ブースト中は Y に触らないので、いつも通り落下する。
//
// ・VelocityComponent は目標速度で、実際の移動は MovementIntegrationSystem が
//   MovementComponent の加速度/減速度で追従させる。ここで差し替えても急にワープはしない。
//   (水平だけが加減速の対象。上下は目標速度がそのまま出る)
//
// ・踏み込み(タップブースト)
//   押した瞬間だけ倍率を掛けても効かない。目標速度は実速度が加速度で追いかける作りで、
//   プレイヤーの加速度は 150 前後。1フレーム(1/60秒)では 2.5m/秒 ほどしか乗らないうちに
//   目標が元へ戻ってしまうので、倍率がまるごと均されて消えていた。
//
//   直すにあたって2つ入れてある。
//     (1) 蹴り出した瞬間は実速度(MovementComponent.velocity)へ直接書いて、
//         その場で最高速へ乗せる(ChargeDashSystem と同じ手)
//     (2) tapBoostTime のあいだ目標速度を倍率から boostPower まで落としていく。
//         実速度はそれを追いかけるので、一番速い瞬間から徐々に遅くなる
//
//   踏み込みの向きは蹴り出したときのものを使う。毎フレーム入力から取り直すと、
//   途中でスティックを離した瞬間に「入力なし = 真上」へ向きが飛ぶ。
//==============================================================================
void RobotBoostSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<BoostComponent, VelocityComponent, const MoveIntentComponent,
		const LookAngleComponent>(
		Engine::ECS::ESystemType::Physics,
		"RobotBoostSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			BoostComponent* a_boostArray,
			VelocityComponent* a_velArray,
			const MoveIntentComponent* a_moveIntentArray,
			const LookAngleComponent* a_lookArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoostComponent&              _boostComp  = a_boostArray[_i];
				VelocityComponent&           _velComp    = a_velArray[_i];
				const MoveIntentComponent&   _moveIntent = a_moveIntentArray[_i];
				const LookAngleComponent&    _lookComp   = a_lookArray[_i];

				// 実際に飛べたかどうかは毎フレームここで決め直す
				_boostComp.isBoosting = false;

				//----------------------------------------------------------
				// 燃料の回復
				//
				// 押している間は回復しない。
				// 回復と消費が同じフレームで走ると、実際に減るのは差ぶんだけになり、
				// 「回復量を上回る消費を設定しないと永久に飛べる」状態になっていた。
				// 押してから離すまでは回復を止めて、消費した量がそのまま減るようにする。
				//
				// 上限で頭打ちにするのも兼ねる。以前は超えてから止めていたので、
				// 満タンのフレームに1回ぶん余分に足されて max をわずかに超えていた
				//----------------------------------------------------------
				if (!_boostComp.isBoostIntent)
				{
					_boostComp.currentFuel = (std::min)(
						_boostComp.currentFuel + _boostComp.fuelRegeneration * a_ctx.dt,
						_boostComp.maxFuel);
				}

				// 踏み込みの残り時間を進める
				if (_boostComp.tapBoostTimer > 0.0f)
				{
					_boostComp.tapBoostTimer = (std::max)(0.0f, _boostComp.tapBoostTimer - a_ctx.dt);
				}

				// 押した瞬間と押しっぱなしは同じフレームで両方成立するので、初動を優先する
				const bool _isTap  = _boostComp.isBoostTriger;
				const bool _isHold = _boostComp.isBoostIntent;

				// 今このフレームで吹かす入力が来ているか
				const bool _isBoostInput = (_isTap || _isHold);

				// 踏み込みが残っている間は、離していても水平の押し出しだけ続ける
				const bool _isTapActive = (_boostComp.tapBoostTimer > 0.0f);

				if (!_isBoostInput && !_isTapActive) continue;

				// 使用量より燃料が下回っていたらブーストできない。
				// 踏み込みは蹴り出したときに払い済みなので、残っている間は止めない
				if (!_isTapActive && _boostComp.currentFuel <= _boostComp.boostFuel) continue;

				//----------------------------------------------------------
				// 進む向きを決める
				//----------------------------------------------------------
				// 上下は入力だけで決めるので Pitch は使わない
				const float _yawRad = DirectX::XMConvertToRadians(_lookComp.Yaw);

				const float _sinY = std::sin(_yawRad);
				const float _cosY = std::cos(_yawRad);

				// 移動入力(前後左右)は水平のまま合成する。
				// 前後の軸から Pitch を抜いてあるので、視点を上下しても水平に滑る
				Math::Vector3 _dir =
					Math::Vector3(_sinY, 0.0f, _cosY) * _moveIntent.value.z +
					Math::Vector3(_cosY, 0.0f, -_sinY) * _moveIntent.value.x;

				// 上下を付けるのは「上下の入力があるとき」と「入力がまったく無いとき」だけ
				bool _applyVertical = false;

				// 上下入力(MoveIntent.y ＝ ジャンプで上、急降下で下)ぶんを足す
				if (std::fabs(_moveIntent.value.y) > 1e-6f)
				{
					_dir.y += _moveIntent.value.y;
					_applyVertical = true;
				}

				// 入力がまったく無いなら真上へ飛ぶ。
				// 視点の前方ではなくワールドの上を使うので、
				// 下を向いていても「何も入れずに吹かせば上がる」で固定できる。
				// 下がりたいときは急降下(LCtrl)を入れて上下入力の側で決める
				//
				// ※ 押しているときだけ。踏み込みの余韻で入力を離しているだけのフレームまで
				//    上昇に振ると、蹴り出した後にボタンを離すと勝手に浮き上がる
				if (_isBoostInput && _dir.LengthSquared() <= 1e-6f)
				{
					_dir = { 0.0f, 1.0f, 0.0f };
					_applyVertical = true;
				}

				// 向きが決まらないときは、踏み込みが残っていればそちらだけで押し出す
				const float _lenSq = _dir.LengthSquared();
				if (_lenSq > 1e-6f) _dir /= std::sqrt(_lenSq);
				else                _dir = { 0.0f, 0.0f, 0.0f };

				//----------------------------------------------------------
				// 蹴り出し : 燃料を払って踏み込みを始める
				//
				// 向きも一緒に覚える。毎フレーム入力から取り直すと、
				// 途中で入力を切った瞬間に向きが飛んでしまう
				//----------------------------------------------------------
				if (_isTap)
				{
					_boostComp.currentFuel -= _boostComp.boostFuel;
					_boostComp.tapBoostTimer = (std::max)(_boostComp.tapBoostTime, 0.0f);
					_boostComp.tapBoostDir = { _dir.x, 0.0f, _dir.z };
				}

				//----------------------------------------------------------
				// 水平の押し出し
				//
				// 踏み込みが残っている間は、蹴り出した向きへ
				// tapBoostScale 倍から boostPower まで落としながら押す。
				// 一番速いのは蹴り出した瞬間で、そこから徐々に遅くなる。
				//
				// 踏み込みが切れていれば、押している間だけ boostPower で押す
				//----------------------------------------------------------
				const bool _isTapPush =
					(_boostComp.tapBoostTimer > 0.0f) &&
					(_boostComp.tapBoostTime > 0.0f) &&
					(_boostComp.tapBoostDir.LengthSquared() > 1e-6f);

				if (_isTapPush)
				{
					// 1(蹴り出した瞬間) → 0(踏み込み終わり)
					const float _tapRate =
						std::clamp(_boostComp.tapBoostTimer / _boostComp.tapBoostTime, 0.0f, 1.0f);

					const float _speed = _boostComp.boostPower *
						(1.0f + (_boostComp.tapBoostScale - 1.0f) * _tapRate);

					_velComp.value.x = _boostComp.tapBoostDir.x * _speed;
					_velComp.value.z = _boostComp.tapBoostDir.z * _speed;
				}
				else if (_isBoostInput && (_dir.x != 0.0f || _dir.z != 0.0f))
				{
					_velComp.value.x = _dir.x * _boostComp.boostPower;
					_velComp.value.z = _dir.z * _boostComp.boostPower;
				}

				// 押しっぱなしの継続ぶん。踏み込み中は蹴り出しで払い済みなので取らない
				if (_isHold && !_isTap && !_isTapPush)
				{
					_boostComp.currentFuel -= _boostComp.boostFuelPerSec * a_ctx.dt;
				}

				//----------------------------------------------------------
				// 蹴り出した瞬間だけ実速度へも直接入れる
				//
				// 目標速度だけだと、実速度が乗るまでに加速度ぶんの時間がかかる。
				// 踏み込みは「押した瞬間が一番速い」ことに意味があるので、
				// 加速を飛ばしてその場で最高速へ乗せる(ChargeDashSystem と同じ手)。
				//
				// MovementComponent をクエリに入れずに RefData で引くのは依存が輪になるため。
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				//----------------------------------------------------------
				if (_isTap && _isTapPush && a_ctx.pWorld)
				{
					const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

					if (a_ctx.pWorld->HasComponent<MovementComponent>(_self))
					{
						if (auto* _pMovement = a_ctx.pWorld->RefData<MovementComponent>(_self))
						{
							_pMovement->velocity.x = _velComp.value.x;
							_pMovement->velocity.z = _velComp.value.z;
						}
					}
				}

				// 上下は該当する場合だけ。触らない間は Y が残るので、
				// 直前に GravitySystem が足した落下ぶんがそのまま効く
				//
				// ※ 初動の tapBoostScale は上下には掛けない。
				//   水平は MovementIntegrationSystem の加減速が初動を均してくれるので
				//   倍率が「踏み込み」として効くが、上下は目標速度がそのまま実速度に
				//   なるため、倍率ぶんがそのフレームの移動量に直接乗る。
				//   boostPower 30 / tapBoostScale 2 / 60fps だと1フレームで 1m 上へ跳び、
				//   上入力だけでブーストしたときに瞬間移動して見えていた。
				// ※ 押しているフレームだけ。踏み込みの余韻で離しているだけのときに
				//   上下を書くと、蹴り出した後に手を離しても浮き続ける
				if (_applyVertical && _isBoostInput)
				{
					_velComp.value.y = _dir.y * _boostComp.boostPower;
				}

				// 噴射の演出と音はここを見ている。
				// 踏み込みの余韻だけのフレームは「吹かしている」ことにしない
				_boostComp.isBoosting = _isBoostInput;
			}
		}
	);
}
