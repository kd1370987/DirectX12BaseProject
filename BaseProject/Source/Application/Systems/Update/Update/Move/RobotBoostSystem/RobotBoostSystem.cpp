#include "RobotBoostSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Character/LookAngleComponent.h"

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

				// 回復
				if (_boostComp.maxFuel >= _boostComp.currentFuel)
				{
					_boostComp.currentFuel += _boostComp.fuelRegeneration * a_ctx.dt;
				}

				// 押されていなければ何もしない
				// (押した瞬間と押しっぱなしは同じフレームで両方成立するので、初動を優先する)
				const bool _isTap  = _boostComp.isBoostTriger;
				const bool _isHold = _boostComp.isBoostIntent;
				if (!_isTap && !_isHold) continue;

				// 使用量より燃料が下回っていたらブーストできない
				if (_boostComp.currentFuel <= _boostComp.boostFuel) continue;

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
				if (_dir.LengthSquared() <= 1e-6f)
				{
					_dir = { 0.0f, 1.0f, 0.0f };
					_applyVertical = true;
				}

				// 向きが決まらなければ吹かさない
				float _lenSq = _dir.LengthSquared();
				if (!(_lenSq > 1e-6f)) continue;
				_dir /= std::sqrt(_lenSq);

				//----------------------------------------------------------
				// 推力
				//----------------------------------------------------------
				float _speed = 0.0f;
				if (_isTap)
				{
					// 初動は tapBoostScale の分だけ速く
					_speed = _boostComp.boostPower * _boostComp.tapBoostScale;
					_boostComp.currentFuel -= _boostComp.boostFuel;
				}
				else
				{
					_speed = _boostComp.boostPower;
					_boostComp.currentFuel -= _boostComp.boostFuelPerSec * a_ctx.dt;
				}

				// 水平の目標速度を差し替える
				_velComp.value.x = _dir.x * _speed;
				_velComp.value.z = _dir.z * _speed;

				// 上下は該当する場合だけ。触らない間は Y が残るので、
				// 直前に GravitySystem が足した落下ぶんがそのまま効く
				//
				// ※ 初動の tapBoostScale は上下には掛けない。
				//   水平は MovementIntegrationSystem の加減速が初動を均してくれるので
				//   倍率が「踏み込み」として効くが、上下は目標速度がそのまま実速度に
				//   なるため、倍率ぶんがそのフレームの移動量に直接乗る。
				//   boostPower 30 / tapBoostScale 2 / 60fps だと1フレームで 1m 上へ跳び、
				//   上入力だけでブーストしたときに瞬間移動して見えていた。
				if (_applyVertical)
				{
					_velComp.value.y = _dir.y * _boostComp.boostPower;
				}

				_boostComp.isBoosting = true;
			}
		}
	);
}
