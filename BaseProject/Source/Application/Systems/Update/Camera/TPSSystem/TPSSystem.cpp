#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "../../../../Components/Camera/TPSFollowComponent.h"
#include "../../../../Components/Camera/CameraFocusTargetComponent.h"
#include "../../../../Components/Camera/CameraDeadZoneComponent.h"
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
//------------------------------------------------------------------------------------------
// 向きと位置を分けて考える
//------------------------------------------------------------------------------------------
// ・**向きは視点角(LookAngle)だけで決まる**。クランプ済みの Yaw/Pitch から作った
//   オービット回転をそのままカメラの姿勢にする。
//   注視点への LookAt では作らない。構図オフセットで横へずらした点を
//   ワールドUpと一緒に LookAt へ渡すと、視線が真上に近づくほどヨーが暴れて
//   「上を向くと左右にそれる」挙動になるため(2026-08-17 修正)。
//   こうしておくとマウスの移動量と画面の回り方が常に 1 対 1 になり、
//   自機がどう動いてもカメラが回らないので照準がブレない。
//
// ・**位置は自機を追いかける**。ただし毎フレーム貼り付くのではなく、
//   自機が画面のデッドゾーン(CameraDeadZoneComponent)から出たぶんだけ
//   カメラを平行移動させて押し戻す。枠の中に居る間カメラは動かない。
//
// ・**注視点(CameraFocusTargetComponent::offsetPos)は既定の構図を決める**。
//   カメラ空間でカメラ自身をずらす量として使い、自機が画面のどこに映るかを決める。
//   +X なら自機は画面左へ、+Y なら画面下へ寄る(カメラが右上へずれるため)。
//   デッドゾーンはこの既定位置を中心とした許容範囲になる。
//
// 補間はすべてフレームレート非依存の指数減衰で行う。
//   t = 1 - exp(-rate * dt)
// よくある t = rate * dt は dt が伸びると t > 1 になって行き過ぎ、
// フレームレートによって追従の速さが変わってしまう。
//
//------------------------------------------------------------------------------------------
// 速度レスポンス(スピード感)
//------------------------------------------------------------------------------------------
// ターゲットの「実速度」を 0..1(speed01)へ正規化して、次の2つを動かす。
//
//   1) 引き  … speed × speedPullBack だけ後方へ(上限 maxPullBack)
//   2) 画角  … fovAddAtSpeed まで広角へ。CameraParamComponent.fovBoost へ書き、
//              射影行列の作り直しは CameraProjUpdateSystem が行う。
//
// どちらもカメラを回さないので照準には影響しない。
// 以前あった「速度で追従を鈍らせて機体を画面端へ流す」効き
// (followRateScale / maxLagAtSpeed / lookAtLagRatio)はデッドゾーンに置き換えた。
// TPSFollowComponent のフィールドは保存データの互換のために残してあるが、
// デッドゾーンを持つカメラでは読まれない。
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
				Math::Quaternion _targetRot = Math::Quaternion::CreateFromYawPitchRoll(
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
				Math::Vector3 _goalPivot		= Math::Vector3(_targetTRS->pos) + Math::Vector3::Up() * _offsetComp.y;
				Math::Vector3 _goalLookAtLocal	= Math::Vector3(_forcusTarget->offsetPos);

				//============================================================
				// ターゲットの速さ(引き/追従/画角のすべての効きの元)
				//------------------------------------------------------------
				// MovementComponent の実速度を優先する。持っていなければ
				// 目標速度(VelocityComponent)で代用し、それも無ければ 0。
				//
				// 上下成分は verticalSpeedWeight で混ぜる。空中戦の上昇/降下でも
				// スピード感が出るが、重みを下げればただの落下ではあまり効かない。
				//============================================================
				Math::Vector3 _targetVel = {};
				if (a_ctx.pWorld->HasComponent<MovementComponent>(_target))
				{
					if (const auto* _pMove = a_ctx.pWorld->RefData<MovementComponent>(_target))
					{
						_targetVel = Math::Vector3(_pMove->velocity);
					}
				}
				else if (a_ctx.pWorld->HasComponent<VelocityComponent>(_target))
				{
					if (const auto* _pVel = a_ctx.pWorld->RefData<VelocityComponent>(_target))
					{
						_targetVel = Math::Vector3(_pVel->value);
					}
				}

				const float _vWeight = std::clamp(_followParam.verticalSpeedWeight, 0.0f, 1.0f);
				const float _rawSpeed = Math::Vector3(
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

				//============================================================
				// カメラまでの距離(符号付き)
				//------------------------------------------------------------
				// offset.z は「前方向にどれだけ進めるか」なので、後方から見る設定では
				// 負の値になっている。引きは符号を合わせて足す(=さらに遠ざける)。
				// デッドゾーンの判定でも使うので、ピボットより先に求めておく。
				//============================================================
				const float _distanceSign = (_offsetComp.z < 0.0f) ? -1.0f : 1.0f;

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
				// 引き量の追従
				//------------------------------------------------------------
				// 速度そのものではなく、遅れて伸び縮みする値にする。
				// 加速した瞬間に引き切ってしまうと「引いている」感じが出ない。
				//============================================================
				_statComp.currentPullBack +=
					(_goalPullBack - _statComp.currentPullBack) * DampFactor(_followParam.pullBackRate, a_ctx.dt);

				// ピボットからカメラまでの距離(符号付き)。デッドゾーンの判定でも使う
				const float _distanceToCamera =
					_offsetComp.z + _distanceSign * _statComp.currentPullBack;

				//============================================================
				// オービット回転の補間(クォータニオンSlerp)
				//------------------------------------------------------------
				// これがそのままカメラの姿勢になる。元は視点角(クランプ済み)なので、
				// 自機の動きでは絶対に回らない = マウスの入力量と画面の回転が一致する。
				//
				// Vector3::Lerp+正規化での「向き」補間は、現在向きと目標向きが
				// ほぼ反対を向いた瞬間に補間結果がゼロ近傍を通り、正規化が破綻して
				// 高速に180度反転する。Slerpは最短経路で回るためこの破綻が無い。
				//============================================================
				Math::Quaternion _curOrbit = _statComp.currentOrbit;
				if (_curOrbit.LengthSquared() < 1e-6f) _curOrbit = _targetRot;	// 未初期化保険

				Math::Quaternion _orbit = Math::Quaternion::Slerp(
					_curOrbit, _targetRot, DampFactor(_followParam.orbitRate, a_ctx.dt));
				_orbit.Normalize();
				_statComp.currentOrbit = _orbit;

				// カメラ空間の3軸(この回転で作った基底)
				const Math::Vector3 _camRight   = Math::Vector3::Transform(Math::Vector3::Right(), _orbit);
				const Math::Vector3 _camUp      = Math::Vector3::Transform(Math::Vector3::Up(), _orbit);
				const Math::Vector3 _camForward = Math::Vector3::Transform(Math::Vector3::Forward(), _orbit);

				//============================================================
				// 注視点(構図)の追従
				//------------------------------------------------------------
				// カメラ空間のオフセットなので、そのまま補間してよい。
				// これは「自機を画面のどこに置くか」を決める既定の構図で、
				// 実際にカメラをこの分だけずらす(自機は逆側へ寄って見える)。
				//============================================================
				Math::Vector3 _lookAtLocal = Math::Vector3(_statComp.currentLookAt);
				_lookAtLocal += (_goalLookAtLocal - _lookAtLocal)
					* DampFactor(_followParam.lookAtRate, a_ctx.dt);

				//============================================================
				// ピボットの追従(デッドゾーン)
				//------------------------------------------------------------
				// 自機が画面の枠の中に居る間はカメラを動かさない。
				// 出たぶんだけカメラ空間で平行移動させて枠の内側へ押し戻す。
				// 向きは一切触らないので、追従中も照準はブレない。
				//
				// デッドゾーンを持たないカメラ(旧データ)は、
				// 今までどおり指数減衰で自機へ寄せる。
				//============================================================
				Math::Vector3 _pivot = _statComp.currentPivot;

				auto* _pDeadZone =
					a_ctx.pWorld->HasComponent<CameraDeadZoneComponent>(a_pChunk->entityData[_i])
					? a_ctx.pWorld->RefData<CameraDeadZoneComponent>(a_pChunk->entityData[_i])
					: nullptr;

				if (_pDeadZone)
				{
					// ---- 画面のどこに映っているかを求める ----
					// カメラは pivot から見て「構図オフセット + 後方へ距離」の位置にある。
					// 自機(=goalPivot)をカメラ空間へ落とせば、割り算だけで NDC が出る。
					const Math::Vector3 _toTarget = _goalPivot - _pivot;

					// ワールド → カメラ空間(回転の逆をかける)
					const Math::Quaternion _orbitConj = _orbit.Conjugate();
					Math::Vector3 _targetCam = Math::Vector3::Transform(_toTarget, _orbitConj);

					// カメラ自身のずれ(構図オフセットと引き)を引いて、カメラ原点基準にする
					_targetCam -= _lookAtLocal;
					_targetCam.z -= _distanceToCamera;

					// 画角から NDC へ。z が手前(0以下)のときは計算できないので追従だけさせる
					const float _tanY = std::tan(DirectX::XMConvertToRadians(_camParamComp.GetFovY()) * 0.5f);
					const float _tanX = _tanY * std::max(_camParamComp.aspectRatio, 1e-4f);

					Math::Vector2 _ndc = { 0.0f, 0.0f };
					const bool _isFront = (_targetCam.z > 1e-3f) && (_tanX > 1e-6f) && (_tanY > 1e-6f);
					if (_isFront)
					{
						_ndc.x = (_targetCam.x / _targetCam.z) / _tanX;
						_ndc.y = (_targetCam.y / _targetCam.z) / _tanY;
					}

					// ---- 枠からはみ出したぶんを、カメラ空間の移動量に直す ----
					const Math::Vector2 _half = _pDeadZone->GetSafeHalfExtents();

					Math::Vector3 _pushCam = {};
					bool _isOutside = false;

					if (_isFront)
					{
						// はみ出し量(NDC) → カメラ空間の距離
						if (_ndc.x > _half.x)
						{
							_pushCam.x = (_ndc.x - _half.x) * _tanX * _targetCam.z;
							_isOutside = true;
						}
						else if (_ndc.x < -_half.x)
						{
							_pushCam.x = (_ndc.x + _half.x) * _tanX * _targetCam.z;
							_isOutside = true;
						}

						if (_ndc.y > _half.y)
						{
							_pushCam.y = (_ndc.y - _half.y) * _tanY * _targetCam.z;
							_isOutside = true;
						}
						else if (_ndc.y < -_half.y)
						{
							_pushCam.y = (_ndc.y + _half.y) * _tanY * _targetCam.z;
							_isOutside = true;
						}
					}
					else
					{
						// 後ろに回り込まれた。枠では直せないので水平だけ一気に合わせる
						_pushCam.x = _targetCam.x;
						_pushCam.y = _targetCam.y;
						_isOutside = true;
					}

					// ---- 奥行き ----
					// 画面の枠では前後を直せないので、離れ/寄りすぎだけ別に詰める
					const float _depthDiff = _targetCam.z - _distanceToCamera * -1.0f;
					const float _depthTol  = std::max(_pDeadZone->depthTolerance, 0.0f);
					float _pushDepth = 0.0f;
					if (_depthDiff > _depthTol)       _pushDepth = _depthDiff - _depthTol;
					else if (_depthDiff < -_depthTol) _pushDepth = _depthDiff + _depthTol;

					// ---- 実際に寄せる ----
					const float _tXY = DampFactor(_pDeadZone->followRate, a_ctx.dt);
					const float _tZ  = DampFactor(_pDeadZone->depthFollowRate, a_ctx.dt);

					_pivot += (_camRight * _pushCam.x + _camUp * _pushCam.y) * _tXY;
					_pivot += _camForward * (_pushDepth * _tZ);

					// 離れすぎたら枠を無視して一気に寄せる(テレポート対策)
					if (_pDeadZone->snapDistance > 0.0f &&
						(_goalPivot - _pivot).Length() > _pDeadZone->snapDistance)
					{
						_pivot = _goalPivot;
					}

					// 確認用に結果を残す
					_pDeadZone->currentNdc = _ndc;
					_pDeadZone->isOutside  = _isOutside;
				}
				else
				{
					// デッドゾーン未設定 : 従来どおり指数減衰で寄せる
					const float _tH = DampFactor(_followParam.posRateHorizontal, a_ctx.dt);
					const float _tV = DampFactor(_followParam.posRateVertical, a_ctx.dt);

					_pivot.x += (_goalPivot.x - _pivot.x) * _tH;
					_pivot.z += (_goalPivot.z - _pivot.z) * _tH;
					_pivot.y += (_goalPivot.y - _pivot.y) * _tV;
				}

				_statComp.currentPivot = _pivot;

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
				// カメラ位置
				//------------------------------------------------------------
				// ピボットの後方へ距離d、そこから構図オフセットぶんカメラ空間でずらす。
				// 注視点を「見る点」ではなく「カメラをずらす量」として使うので、
				// どれだけ上下を向いても姿勢は視点角のまま = 横にそれない。
				//============================================================
				Math::Vector3 _currentPos =
					_pivot
					+ _camForward * _distanceToCamera
					+ _camRight * _lookAtLocal.x
					+ _camUp * _lookAtLocal.y
					+ _camForward * _lookAtLocal.z;

				//============================================================
				// 保存
				//------------------------------------------------------------
				// カメラの姿勢はオービット回転そのもの。
				// LookAt を通さないので、真上/真下でも破綻しない。
				//============================================================
				_trsComp.pos = _currentPos;
				_trsComp.quat = _orbit;
				_statComp.currentLookAt = _lookAtLocal;	// 保持するのはカメラ空間の相対座標

				// 解決後の注視点(他システム参照用)。カメラが実際に向いている先
				_statComp.lookAtWorld = _pivot + Math::Vector3::Transform(_lookAtLocal, _orbit);

				_trsComp.isDirty = true;
			}
		}
	);
}
