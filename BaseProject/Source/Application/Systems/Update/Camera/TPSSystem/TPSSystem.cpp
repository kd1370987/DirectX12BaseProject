#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "../../../../Components/Camera/TPSFollowComponent.h"
#include "../../../../Components/Camera/CameraFocusTargetComponent.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/MovementComponent.h"


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
//
//------------------------------------------------------------------------------------------
// 速度レスポンス(スピード感)
//------------------------------------------------------------------------------------------
// ターゲットの「実速度」を 0..1(speed01)へ正規化し、次の4つをまとめて動かす。
// 速いほどカメラが機体に追いつけなくなり、画角が広がる、という一本の軸にしてある。
//
//   1) 引き    … speed × speedPullBack だけ後方へ(上限 maxPullBack)
//   2) 追従    … 位置/注視点の追従レートへ followRateScale を掛けて鈍らせる
//   3) 遅れ幅  … 許すピボットの遅れを maxLagDistance → maxLagAtSpeed へ広げる
//   4) 注視点  … 注視点の基準をターゲットの現在位置から「遅れたピボット」側へ寄せる
//                (lookAtLagRatio)。ここが 0 だと機体は常に画面中央から動かない。
//                上げるほど進行方向と反対の画面端へ機体が流れ、見切れる寸前になる。
//   5) 画角    … fovAddAtSpeed まで広角へ。CameraParamComponent.fovBoost へ書き、
//                射影行列の作り直しは CameraProjUpdateSystem が行う。
//
// 速さは「目標速度(VelocityComponent)」ではなく MovementComponent の実速度を使う。
// 目標速度は入力やブーストで 1 フレームで飛ぶので、そのまま使うと画角と引きが
// 階段状に切り替わる。実速度なら加減速のカーブがそのままスピード感になる。
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
	a_world.ActiveTask<FollowTargetComponent, TPSOffsetComponent, TPSLookAngleComponent, const TPSFollowComponent, LocalTransformComponent, TPSCameraStateComponent, CameraParamComponent>(
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
			TPSCameraStateComponent* a_tpsStatArray,
			CameraParamComponent* a_camParamArray
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
				CameraParamComponent&		_camParamComp	= a_camParamArray[_i];

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
				// ターゲットの速さ(引き/追従/画角のすべての効きの元)
				//------------------------------------------------------------
				// MovementComponent の実速度を優先する。持っていなければ
				// 目標速度(VelocityComponent)で代用し、それも無ければ 0。
				//
				// 上下成分は verticalSpeedWeight で混ぜる。空中戦の上昇/降下でも
				// スピード感が出るが、重みを下げればただの落下ではあまり効かない。
				//============================================================
				DXSM::Vector3 _targetVel = {};
				if (a_ctx.pWorld->HasComponent<MovementComponent>(_target))
				{
					if (const auto* _pMove = a_ctx.pWorld->RefData<MovementComponent>(_target))
					{
						_targetVel = DXSM::Vector3(_pMove->velocity);
					}
				}
				else if (a_ctx.pWorld->HasComponent<VelocityComponent>(_target))
				{
					if (const auto* _pVel = a_ctx.pWorld->RefData<VelocityComponent>(_target))
					{
						_targetVel = DXSM::Vector3(_pVel->value);
					}
				}

				const float _vWeight = std::clamp(_followParam.verticalSpeedWeight, 0.0f, 1.0f);
				const float _rawSpeed = DXSM::Vector3(
					_targetVel.x, _targetVel.y * _vWeight, _targetVel.z).Length();

				// 0..1 へ正規化。speedReference が 0 以下なら速度レスポンスなし
				const float _rawSpeed01 = (_followParam.speedReference > 1e-4f)
					? std::clamp(_rawSpeed / _followParam.speedReference, 0.0f, 1.0f)
					: 0.0f;

				//============================================================
				// 速度レスポンスは「なまして」から使う
				//------------------------------------------------------------
				// 実速度のうち上下成分は加減速を通さず目標速度がそのまま出る
				// (重力や着地が鈍らないよう MovementIntegrationSystem がそう作ってある)。
				// そのため上下ブーストの瞬間は速さが 1 フレームで跳ね、これを直接
				// 引き・追従レート・遅れ上限・画角へ流すと、カメラが一気に動いて見える。
				// 特に遅れ上限は「そこまで来たら引き戻す」という強い制限なので、
				// 値が急に縮むとピボットがその場で引き寄せられてカクつく。
				//
				// 速さそのものを指数減衰でなましてから配れば、どの効きも滑らかに立ち上がる。
				//============================================================
				_statComp.currentSpeed01 +=
					(_rawSpeed01 - _statComp.currentSpeed01)
					* DampFactor(_followParam.speedResponseRate, a_ctx.dt);

				const float _speed01 = _statComp.currentSpeed01;

				// 引きの量も、なました速さから作る(跳ねをそのまま距離にしない)
				float _goalPullBack = std::clamp(
					_speed01 * _followParam.speedReference * _followParam.speedPullBack,
					0.0f,
					std::max(_followParam.maxPullBack, 0.0f));

				// 速いほど追従を鈍らせる(1.0 → followRateScale)
				const float _rateScale =
					1.0f + (std::clamp(_followParam.followRateScale, 0.0f, 1.0f) - 1.0f) * _speed01;

				// 速いほど遅れてよい距離を広げる
				const float _maxLag =
					_followParam.maxLagDistance +
					(_followParam.maxLagAtSpeed - _followParam.maxLagDistance) * _speed01;

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
					_statComp.currentSpeed01	= _rawSpeed01;
					_statComp.currentFovAdd		= _followParam.fovAddAtSpeed * _rawSpeed01;
					_statComp.isInitialized		= true;
				}

				//============================================================
				// ピボットの追従
				//------------------------------------------------------------
				// 水平と垂直で追従速度を分ける。垂直を遅くしておくと、
				// 上昇/落下でカメラがすぐに持ち上がらず柔らかい見え方になる。
				//
				// 速度で鈍らせる(followRateScale)のは水平だけ。垂直まで鈍らせると、
				// 上下ブーストで機体が一気に上下したときにカメラが付いていけず、
				// 遅れ上限まで離れて「引き戻し」が効いた瞬間にガクッと動く。
				// 水平の遅れは「置いていかれる」気持ちよさになるが、
				// 垂直の遅れは機体が画面から出るだけで得がない。
				//============================================================
				DXSM::Vector3 _pivot = _statComp.currentPivot;
				{
					float _tH = DampFactor(_followParam.posRateHorizontal * _rateScale, a_ctx.dt);
					float _tV = DampFactor(_followParam.posRateVertical, a_ctx.dt);

					_pivot.x += (_goalPivot.x - _pivot.x) * _tH;
					_pivot.z += (_goalPivot.z - _pivot.z) * _tH;
					_pivot.y += (_goalPivot.y - _pivot.y) * _tV;

					// 離れすぎたら引き戻す。
					// テレポートや極端な加速でカメラが千切れたままになるのを防ぐ保険。
					// 上限は速度で広がる(止まっているときは maxLagDistance)。
					DXSM::Vector3 _lag = _pivot - _goalPivot;
					float _lagLen = _lag.Length();
					if (_maxLag > 0.0f && _lagLen > _maxLag)
					{
						_pivot = _goalPivot + _lag * (_maxLag / _lagLen);
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
				// ワールドへ戻すときの基準は、ターゲットの実際の位置と
				// 「遅れているピボット」を speed01 × lookAtLagRatio で混ぜたもの。
				//   ターゲット基準 … 機体は常に画面中央。構図は安定するが速さが出ない
				//   ピボット基準   … 遅れたぶんだけ機体が進行方向側の画面端へ流れる
				// 止まっているときは前者に一致するので、通常時の構図は変わらない。
				// 回転はオービットなので、機体の向きには一切影響されない。
				//============================================================
				DXSM::Vector3 _lookAtLocal = DXSM::Vector3(_statComp.currentLookAt);
				_lookAtLocal += (_goalLookAtLocal - _lookAtLocal)
					* DampFactor(_followParam.lookAtRate * _rateScale, a_ctx.dt);

				// ピボットは offset.y ぶん持ち上げてあるので、比較できるよう戻してから混ぜる
				DXSM::Vector3 _laggedPos  = _pivot - DXSM::Vector3::Up * _offsetComp.y;
				const float   _lookAtLag  = std::clamp(_followParam.lookAtLagRatio, 0.0f, 1.0f) * _speed01;
				DXSM::Vector3 _lookAtBase = DXSM::Vector3::Lerp(
					DXSM::Vector3(_targetTRS->pos), _laggedPos, _lookAtLag);

				DXSM::Vector3 _lookAt = _lookAtBase + DXSM::Vector3::Transform(_lookAtLocal, _orbit);

				//============================================================
				// 引き量の追従
				//------------------------------------------------------------
				// 速度そのものではなく、遅れて伸び縮みする値にする。
				// 加速した瞬間に引き切ってしまうと「引いている」感じが出ない。
				//============================================================
				_statComp.currentPullBack +=
					(_goalPullBack - _statComp.currentPullBack) * DampFactor(_followParam.pullBackRate, a_ctx.dt);

				//============================================================
				// 画角の追従
				//------------------------------------------------------------
				// 速いほど広角にして、周りの流れる速さでスピード感を出す。
				// 基準の画角(fovY)は触らず、上乗せ分だけを fovBoost へ書く。
				// 実際に射影行列を作り直すのは CameraProjUpdateSystem。
				// 毎フレーム作り直さずに済むよう、意味のある差が出たときだけ dirty にする。
				//============================================================
				const float _goalFovAdd = _followParam.fovAddAtSpeed * _speed01;
				_statComp.currentFovAdd +=
					(_goalFovAdd - _statComp.currentFovAdd) * DampFactor(_followParam.fovRate, a_ctx.dt);

				if (std::fabs(_camParamComp.fovBoost - _statComp.currentFovAdd) > 0.01f)
				{
					_camParamComp.fovBoost = _statComp.currentFovAdd;
					_camParamComp.isDirty  = true;
				}

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
