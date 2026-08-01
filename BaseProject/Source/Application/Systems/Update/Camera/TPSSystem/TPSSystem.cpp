#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "../../../../Components/Camera/TPSFollowComponent.h"
#include "../../../../Components/Camera/CameraFocusTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Force/VelocityComponent.h"


#include "Application/Components/Camera/TPSLookAngleComponent.h"

#include "Application/Components/Character/LookAngleComponent.h"

//==========================================================================================
// TPSSystem
//
// ターゲット(自機)を追いかけるTPSカメラ。
//
// ピボット・注視点・引き量をそれぞれ「遅れて」追従させることで、
// アーマードコアのように機体へ貼り付かず、加速すると置いていかれて
// 少ししてから追いつく挙動になる。調整値は TPSFollowComponent が持つ。
//
// 補間はすべてフレームレート非依存の指数減衰で行う。
//   t = 1 - exp(-rate * dt)
// よくある t = rate * dt は dt が伸びると t > 1 になって行き過ぎ、
// フレームレートによって追従の速さが変わってしまう。
//==========================================================================================
namespace
{
	//--------------------------------------------------------------------------------------
	// 指数減衰の補間係数。rate が 0 以下なら追従しない。
	//--------------------------------------------------------------------------------------
	float DampFactor(float a_rate, float a_dt)
	{
		if (!(a_rate > 0.0f) || !(a_dt > 0.0f)) return 0.0f;
		return 1.0f - std::exp(-a_rate * a_dt);
	}
}

void TPSSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<FollowTargetComponent, TPSOffsetComponent, TPSLookAngleComponent, const TPSFollowComponent, LocalTransformComponent, TPSCameraStateComponent>(
		Engine::ECS::ESystemType::Camera,
		"TPSSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			FollowTargetComponent* a_targetArray,
			TPSOffsetComponent* a_offsetArray,
			TPSLookAngleComponent* a_lookAngArray,
			const TPSFollowComponent* a_followParamArray,
			LocalTransformComponent* a_trsArray,
			TPSCameraStateComponent* a_tpsStatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// カメラのコンポーネントを取得
				FollowTargetComponent&		_followComp		= a_targetArray[_i];
				TPSOffsetComponent&			_offsetComp		= a_offsetArray[_i];
				const TPSFollowComponent&	_followParam	= a_followParamArray[_i];
				LocalTransformComponent&	_trsComp		= a_trsArray[_i];
				TPSLookAngleComponent&		_lookComp		= a_lookAngArray[_i];
				TPSCameraStateComponent&	_statComp		= a_tpsStatArray[_i];

				//============================================================
				// ターゲット取得
				//============================================================
				Engine::ECS::Entity _target = _followComp.target;
				if (!a_ctx.pWorld->HasComponent<LocalTransformComponent>(_target)) continue;
				if (!a_ctx.pWorld->HasComponent<LookAngleComponent>(_target)) continue;
				if (!a_ctx.pWorld->HasComponent<CameraFocusTargetComponent>(_target)) continue;
				const LocalTransformComponent* _targetTRS = a_ctx.pWorld->RefData<LocalTransformComponent>(_target);
				const LookAngleComponent* _targetLook = a_ctx.pWorld->RefData<LookAngleComponent>(_target);
				const CameraFocusTargetComponent* _forcusTarget = a_ctx.pWorld->RefData<CameraFocusTargetComponent>(_target);
				if (!_targetLook || !_targetTRS || !_forcusTarget) continue;

				//============================================================
				// ターゲット回転
				//------------------------------------------------------------
				// Yaw/Pitchは度(degree)で保持されているのでラジアンへ変換する。
				// Vector3オーバーロードは(pitch,yaw,roll)順で軸が入れ替わるため、
				// スカラー版 CreateFromYawPitchRoll(yaw,pitch,roll) を明示的に使う。
				//============================================================
				DXSM::Quaternion _targetRot = DXSM::Quaternion::CreateFromYawPitchRoll(
					DirectX::XMConvertToRadians(_targetLook->Yaw),
					DirectX::XMConvertToRadians(-_targetLook->Pitch),
					0.0f
				);
				_targetRot.Normalize();

				//============================================================
				// 追従の目標値
				//------------------------------------------------------------
				// 注視点のオフセットは「カメラ空間(オービット基準)」で扱う。
				// 左手系なので +Z が視線の奥、+X が画面右、+Y が画面上。
				//   例) 機体を画面の左下に置いて右上を狙うなら x,y を正にする。
				//
				// 機体の姿勢(LocalTransform の quat)で回してはいけない。
				// プレイヤーの胴体は LockOnRotationSystem が狙点/進行方向へ
				// Slerp で向けるため視線角と一致せず、横移動や旋回のたびに
				// 「機体の右上」がカメラから見て回り込み、それを追ってカメラが
				// 振られる。カメラ空間で持てば機体がどちらを向いても
				// 画面内の構図は変わらない。
				//============================================================
				DXSM::Vector3 _goalPivot		= DXSM::Vector3(_targetTRS->pos) + DXSM::Vector3::Up * _offsetComp.y;
				DXSM::Vector3 _goalLookAtLocal	= DXSM::Vector3(_forcusTarget->offsetPos);

				//============================================================
				// 水平速度(引きの量に使う)
				//------------------------------------------------------------
				// 落下速度まで含めると、ただ落ちているだけでカメラが引いてしまうので
				// 水平成分だけを見る。速度を持たないターゲットなら 0 として扱う。
				//============================================================
				float _horizontalSpeed = 0.0f;
				if (a_ctx.pWorld->HasComponent<VelocityComponent>(_target))
				{
					if (const auto* _pVel = a_ctx.pWorld->RefData<VelocityComponent>(_target))
					{
						_horizontalSpeed = DXSM::Vector2(_pVel->value.x, _pVel->value.z).Length();
					}
				}
				float _goalPullBack = std::clamp(
					_horizontalSpeed * _followParam.speedPullBack,
					0.0f,
					std::max(_followParam.maxPullBack, 0.0f));

				//============================================================
				// 初回はスナップ
				//------------------------------------------------------------
				// 未初期化のまま補間すると、原点(あるいは保存時の座標)から
				// カメラが飛んでくる。シーン開始直後だけ目標値をそのまま入れる。
				//============================================================
				if (!_statComp.isInitialized)
				{
					_statComp.currentPivot		= _goalPivot;
					_statComp.currentLookAt		= _goalLookAtLocal;
					_statComp.currentOrbit		= _targetRot;
					_statComp.currentPullBack	= _goalPullBack;
					_statComp.isInitialized		= true;
				}

				//============================================================
				// ピボットの追従
				//------------------------------------------------------------
				// 水平と垂直で追従速度を分ける。垂直を遅くしておくと、
				// 上昇/落下でカメラがすぐに持ち上がらず柔らかい見え方になる。
				//============================================================
				DXSM::Vector3 _pivot = _statComp.currentPivot;
				{
					float _tH = DampFactor(_followParam.posRateHorizontal, a_ctx.dt);
					float _tV = DampFactor(_followParam.posRateVertical, a_ctx.dt);

					_pivot.x += (_goalPivot.x - _pivot.x) * _tH;
					_pivot.z += (_goalPivot.z - _pivot.z) * _tH;
					_pivot.y += (_goalPivot.y - _pivot.y) * _tV;

					// 離れすぎたら引き戻す。
					// テレポートや極端な加速でカメラが千切れたままになるのを防ぐ保険。
					DXSM::Vector3 _lag = _pivot - _goalPivot;
					float _lagLen = _lag.Length();
					if (_followParam.maxLagDistance > 0.0f && _lagLen > _followParam.maxLagDistance)
					{
						_pivot = _goalPivot + _lag * (_followParam.maxLagDistance / _lagLen);
					}
				}
				_statComp.currentPivot = _pivot;

				//============================================================
				// オービット回転の補間(クォータニオンSlerp)
				//------------------------------------------------------------
				// Vector3::Lerp+正規化での「向き」補間は、現在向きと目標向きが
				// ほぼ反対を向いた瞬間に補間結果がゼロ近傍を通り、正規化が破綻して
				// 高速に180度反転する。Slerpは最短経路で回るためこの破綻が無い。
				//
				// 注視点のオフセットをこの回転で解決するので、注視点より先に求める。
				//============================================================
				DXSM::Quaternion _curOrbit = _statComp.currentOrbit;
				if (_curOrbit.LengthSquared() < 1e-6f) _curOrbit = _targetRot;	// 未初期化保険

				DXSM::Quaternion _orbit = DXSM::Quaternion::Slerp(
					_curOrbit, _targetRot, DampFactor(_followParam.orbitRate, a_ctx.dt));
				_orbit.Normalize();
				_statComp.currentOrbit = _orbit;

				//============================================================
				// 注視点の追従
				//------------------------------------------------------------
				// 補間はカメラ空間のまま行う。ワールドで補間すると、遅れている間に
				// 機体が動いた分だけ注視点が取り残されて構図が揺れる。
				//
				// ワールドへ戻すときの基準はピボット(遅れて追従する)ではなく
				// ターゲットの実際の位置。ここが遅れると機体が画面から逃げる。
				// 回転はオービットなので、機体の向きには一切影響されない。
				//============================================================
				DXSM::Vector3 _lookAtLocal = DXSM::Vector3(_statComp.currentLookAt);
				_lookAtLocal += (_goalLookAtLocal - _lookAtLocal) * DampFactor(_followParam.lookAtRate, a_ctx.dt);

				DXSM::Vector3 _lookAt =
					DXSM::Vector3(_targetTRS->pos) + DXSM::Vector3::Transform(_lookAtLocal, _orbit);

				//============================================================
				// 引き量の追従
				//------------------------------------------------------------
				// 速度そのものではなく、遅れて伸び縮みする値にする。
				// 加速した瞬間に引き切ってしまうと「引いている」感じが出ない。
				//============================================================
				_statComp.currentPullBack +=
					(_goalPullBack - _statComp.currentPullBack) * DampFactor(_followParam.pullBackRate, a_ctx.dt);

				//============================================================
				// カメラ位置：ピボットの後方へ距離d(常に球面上=距離固定)
				//------------------------------------------------------------
				// offset.z は「前方向にどれだけ進めるか」なので、後方から見る設定では
				// 負の値になっている。引きは符号を合わせて足す(=さらに遠ざける)。
				//============================================================
				float _distanceSign = (_offsetComp.z < 0.0f) ? -1.0f : 1.0f;
				float _distance = _offsetComp.z + _distanceSign * _statComp.currentPullBack;

				DXSM::Vector3 _dir = DXSM::Vector3::Transform(DXSM::Vector3::Backward, _orbit); // (0,0,1)を回転
				DXSM::Vector3 _currentPos = _pivot + _dir * _distance;

				//============================================================
				// カメラ回転(このエンジンは左手系。CreateLookAtは右手系で
				// 向きが180度反転するため XMMatrixLookAtLH を使う)
				//------------------------------------------------------------
				// XMMatrixLookAtLH は「視点==注視点」や「視線がUpと平行」の場合に
				// 内部の正規化/外積が破綻して行列全体が NaN になる。
				// NaN のクォータニオンが LocalTransform に入ると、そこから前方ベクトルを
				// 求める側(AimTargetSystem など)まで NaN が伝播して落ちるため、
				// 破綻する条件では回転を更新せず前フレームの値を保つ。
				//============================================================
				DXSM::Vector3 _lookVec = _lookAt - _currentPos;
				float _lookLenSq = _lookVec.LengthSquared();
				bool _isDegenerate = (_lookLenSq < 1e-6f);
				if (!_isDegenerate)
				{
					// 視線がUpとほぼ平行（真上/真下を向いている）かどうか
					DXSM::Vector3 _lookDir = _lookVec / std::sqrt(_lookLenSq);
					_isDegenerate = (std::fabs(_lookDir.Dot(DXSM::Vector3::Up)) > 0.9999f);
				}
				// 破綻時は前フレームの回転を維持する(位置だけは更新する)
				DXSM::Quaternion _camRot = DXSM::Quaternion(_trsComp.quat);
				if (!_isDegenerate)
				{
					DXSM::Matrix _view = DirectX::XMMatrixLookAtLH(
						_currentPos,
						_lookAt,
						DXSM::Vector3::Up
					);
					_camRot = DXSM::Quaternion::CreateFromRotationMatrix(_view.Invert());
					_camRot.Normalize();
				}

				//============================================================
				// 保存
				//============================================================
				_trsComp.pos = _currentPos;
				_trsComp.quat = _camRot;
				_statComp.currentLookAt = _lookAtLocal;	// 保持するのはカメラ空間の相対座標
				_statComp.lookAtWorld	= _lookAt;		// 解決後のワールド座標(他システム参照用)
				_trsComp.isDirty = true;
			}
		}
	);
}
