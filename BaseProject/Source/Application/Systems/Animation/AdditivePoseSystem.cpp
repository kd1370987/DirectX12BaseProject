#include "AdditivePoseSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Application/Components/Animation/AdditivePoseComponent.h"
#include "Application/Components/Movement/LookAngleComponent.h"
#include "Application/Components/Combat/LockOnTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Movement/ActualVelocityComponent.h"
#include "Application/Components/Physics/GroundStateComponent.h"
#include "Application/Components/Movement/BoostStateComponent.h"
#include "Application/Components/Movement/ChargeDashComponent.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

namespace
{
	//----------------------------------------------------------------------------------
	// 回転行列の前方ベクトルから Yaw / Pitch(ラジアン)を取り出す。
	// このエンジンは行ベクトル規約なので、前方ベクトルは行列の第3行になる。
	//----------------------------------------------------------------------------------
	void ExtractYawPitch(const Math::Quaternion& a_quat, float& a_outYaw, float& a_outPitch)
	{
		Math::Matrix _mat = Math::Matrix::CreateFromQuaternion(a_quat);
		Math::Vector3 _forward(_mat._31, _mat._32, _mat._33);
		_forward.Normalize();

		a_outYaw = std::atan2f(_forward.x, _forward.z);
		a_outPitch = -std::asinf(std::clamp(_forward.y, -1.0f, 1.0f));
	}

	//----------------------------------------------------------------------------------
	// モデル空間で表現した回転を、対象ボーンのローカル空間へ移す。
	//
	// world = local * parentWorld(行ベクトル規約)なので、
	// local' = Mat(qBone) * local とすると qBone はボーン空間で作用し、
	// 関節原点を軸に回る(ボーンの長さや接続位置を壊さない)。
	//
	// ボーンのバインドポーズ姿勢 qBind を使って
	//   qBone = qBind * qModel * conj(qBind)
	// と共役変換すれば、見た目はモデル空間で qModel だけ回したものと一致する。
	//----------------------------------------------------------------------------------
	Math::Quaternion ToBoneSpace(const Math::Quaternion& a_modelQuat, const Math::Matrix& a_bindWorld)
	{
		Math::Matrix _bindMat(a_bindWorld);

		Math::Vector3 _bindScale;
		Math::Quaternion _bindRot;
		Math::Vector3 _bindTrans;
		if (!_bindMat.Decompose(_bindScale, _bindRot, _bindTrans))
		{
			// バインド行列が壊れている場合はモデル空間のまま適用する
			return a_modelQuat;
		}

		const Math::Quaternion _bindConj = _bindRot.Conjugate();

		Math::Quaternion _result = _bindRot * a_modelQuat * _bindConj;
		_result.Normalize();
		return _result;
	}
}

void AdditivePoseSystem::Init(App::ECS::APPWorld& a_world)
{
	// 自分のチャンクとプールの自分の範囲だけを書く(別のエンティティは読むだけ)ので、
	// チャンクを分けてワーカーで回す
	a_world.ActiveJobTask<
		const ModelComponent,
		const AnimatorComponent,
		const LookAngleComponent,
		const LocalTransformComponent,
		const DesiredVelocityComponent,
		NodePoseComponent,
		AdditivePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"AdditivePoseSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			const AnimatorComponent* a_animatorArray,
			const LookAngleComponent* a_lookArray,
			const LocalTransformComponent* a_trsArray,
			const DesiredVelocityComponent* a_velocityArray,
			NodePoseComponent* a_nodePoseArray,
			AdditivePoseComponent* a_additiveArray
		)
		{
			if (a_ctx.dt <= 0.0f) return;

			auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
			auto& _entryPool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				const AnimatorComponent& _animComp = a_animatorArray[_i];
				const LookAngleComponent& _lookComp = a_lookArray[_i];
				const LocalTransformComponent& _trsComp = a_trsArray[_i];
				const DesiredVelocityComponent& _velComp = a_velocityArray[_i];
				NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];
				AdditivePoseComponent& _addComp = a_additiveArray[_i];

				// 対象ボーンが未解決なら何もしない
				auto _entryVec = _entryPool.RefRange(_addComp.handle);
				if (_entryVec.empty()) continue;

				const auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_pModel) continue;

				auto _nodePoseVec = _nodePosePool.RefRange(_nodePoseComp.nodePoseHandle);
				if (_nodePoseVec.empty()) continue;

				const auto& _nodeVec = _pModel->GetOriginalNodeVec();

				//==========================================================================
				// 機体の向き(ワールド)
				//==========================================================================
				Math::Quaternion _bodyQuat(_trsComp.quat);
				if (_bodyQuat.LengthSquared() < 1e-6f) _bodyQuat = Math::Quaternion::Identity();
				_bodyQuat.Normalize();

				// 機体の向きの逆。Conjugate() は「その場で反転」ではなく
				// 反転したものを返すので、必ず受け取ること
				const Math::Quaternion _bodyConj = _bodyQuat.Conjugate();

				//==========================================================================
				// Aim: 照準方向と機体の向きの差分を、モデル空間の回転として求める
				//--------------------------------------------------------------------------
				// 角度は度で保持されているのでラジアンへ変換する。
				// Vector3 オーバーロードは(pitch,yaw,roll)順で軸が入れ替わるため、
				// スカラー版 CreateFromYawPitchRoll(yaw,pitch,roll) を明示的に使う。
				//
				// 既定は視点角(カメラの向き)。ただし視点角には「自分がどこに居るか」が
				// 入っていないので、自機が画面の中央から外れているほど、狙っている相手では
				// なくカメラの正面を向いてしまう。カメラが速度で後ろへ引かれて自機が画面端へ
				// 流れる場面ほど差が開き、しかも左右で符号が逆になるので、
				// 片側だけ正しく見えるという出方をする。
				// そのためロック中は視点角を捨て、「自分 → ロック相手」の向きから作り直す。
				//==========================================================================
				float _aimYaw   = DirectX::XMConvertToRadians(_lookComp.Yaw);
				float _aimPitch = DirectX::XMConvertToRadians(-_lookComp.Pitch);

				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (a_ctx.pWorld->HasComponent<LockOnTargetComponent>(_self))
				{
					if (const auto* _pLockOn = a_ctx.pWorld->RefData<LockOnTargetComponent>(_self))
					{
						if (_pLockOn->IsLocked())
						{
							Math::Vector3 _toTarget =
								Math::Vector3(_pLockOn->lockedPos) - Math::Vector3(_trsComp.pos);

							// 長さのチェックは NaN も弾ける形で書くこと
							float _lenSq = _toTarget.LengthSquared();
							if (_lenSq > 1e-6f)
							{
								_toTarget /= std::sqrt(_lenSq);

								// 左手系 +Z 前方。CreateFromYawPitchRoll の前方は
								//   (sinYaw*cosPitch, -sinPitch, cosYaw*cosPitch)
								// なので、上を向かせたいとき pitch は負になる。
								_aimYaw   = std::atan2(_toTarget.x, _toTarget.z);
								_aimPitch = std::asin(std::clamp(-_toTarget.y, -1.0f, 1.0f));
							}
						}
					}
				}

				Math::Quaternion _lookQuat = Math::Quaternion::CreateFromYawPitchRoll(
					_aimYaw,
					_aimPitch,
					0.0f
				);
				_lookQuat.Normalize();

				// Mat(offset) = Mat(look) * Mat(body)^-1 となる差分
				Math::Quaternion _offsetQuat = _lookQuat * _bodyConj;
				_offsetQuat.Normalize();

				// 可動域でクランプしてから作り直す(ロールは捨てる)
				float _yaw = 0.0f;
				float _pitch = 0.0f;
				ExtractYawPitch(_offsetQuat, _yaw, _pitch);

				const float _yawLimit = DirectX::XMConvertToRadians(_addComp.yawLimitDeg);
				const float _pitchLimit = DirectX::XMConvertToRadians(_addComp.pitchLimitDeg);
				_yaw = std::clamp(_yaw, -_yawLimit, _yawLimit);
				_pitch = std::clamp(_pitch, -_pitchLimit, _pitchLimit);

				Math::Quaternion _targetAim = Math::Quaternion::CreateFromYawPitchRoll(_yaw, _pitch, 0.0f);
				_targetAim.Normalize();

				// Slerpで追従(急に振り向かせない)
				Math::Quaternion _currentAim(_addComp.currentAimQuat);
				if (_currentAim.LengthSquared() < 1e-6f) _currentAim = Math::Quaternion::Identity();

				float _t = std::min(_addComp.followRate * a_ctx.dt, 1.0f);
				_currentAim = Math::Quaternion::Slerp(_currentAim, _targetAim, _t);
				_currentAim.Normalize();
				_addComp.currentAimQuat = _currentAim;

				//==========================================================================
				// Lag: 進行方向の逆へ手足を流す。
				//
				// 駆動するのは加速度ではなく速度。
				// 加速度で駆動すると、減速中(スティックを離した後)は加速度が進行方向と
				// 逆を向くため、まだ同じ方向へ動いているのに流れる向きが反転してしまう。
				// 速度で見れば「動いている方向の逆へ流れる」が常に成り立ち、
				// 止まれば速度と一緒に自然と戻る。
				//
				// 加減速を持つ機体は MovementParamsComponent の実速度を見る。
				// DesiredVelocityComponent は「目標速度」で、入力やブーストで 0 → 30 のように
				// 1フレームで飛ぶ値なので、そのまま使うと流れ始めが階段状になる。
				// 実速度は加速度/減速度で滑らかに変化するので、歩き出しは浅く、
				// ブーストは深く、と速さがそのまま角度に出る。
				//==========================================================================
				Math::Vector3 _velocity(_velComp.value);

				if (a_ctx.pWorld->HasComponent<ActualVelocityComponent>(_self))
				{
					if (const auto* _pMovement = a_ctx.pWorld->RefData<ActualVelocityComponent>(_self))
					{
						_velocity = Math::Vector3(_pMovement->value);
					}
				}

				// ワールド速度をモデル空間へ(x:右 y:上 z:前)
				Math::Vector3 _velModel = Math::Vector3::Transform(_velocity, _bodyConj);

				// 前方向の速度は X軸(右)まわり、横方向の速度は Z軸(前)まわりに倒す。
				// どちらも軸スケールが正のときに「進行方向の逆へ流れる」向きになる
				// (下向きの手足に対して、+X 回転は後ろへ、+Z 回転は右へ振れるため符号が入れ替わる)
				const float _lagLimit = DirectX::XMConvertToRadians(_addComp.lagLimitDeg);
				Math::Vector3 _lagTarget = {};
				_lagTarget.x = std::clamp(_velModel.z * _addComp.lagScale, -_lagLimit, _lagLimit);
				_lagTarget.y = 0.0f;
				_lagTarget.z = std::clamp(-_velModel.x * _addComp.lagScale, -_lagLimit, _lagLimit);

				// バネで追従させ、加速が止まったら自然に戻す
				Math::Vector3 _lagAngle(_addComp.lagAngle);
				Math::Vector3 _lagVel(_addComp.lagVelocity);

				_lagVel += ((_lagTarget - _lagAngle) * _addComp.lagStiffness - _lagVel * _addComp.lagDamping) * a_ctx.dt;
				_lagAngle += _lagVel * a_ctx.dt;

				_lagAngle.x = std::clamp(_lagAngle.x, -_lagLimit, _lagLimit);
				_lagAngle.y = std::clamp(_lagAngle.y, -_lagLimit, _lagLimit);
				_lagAngle.z = std::clamp(_lagAngle.z, -_lagLimit, _lagLimit);

				_addComp.lagAngle = _lagAngle;
				_addComp.lagVelocity = _lagVel;

				//==========================================================================
				// 地上 ⇔ 空中の切り替え
				//
				// 空中用のチャンネル(AimArm / LagBody)を持つものだけ、接地していない間は
				// 地上用の Aim(上半身)を止めて、腕だけで狙い・体全体を流す。
				// 持たないもの(空を飛ぶ敵など)は今まで通り地上用だけを使う。
				// 着地・離陸でポーズが飛ばないよう、airBlend を時間で寄せて混ぜる。
				//==========================================================================
				const bool _hasAirChannel = std::any_of(_entryVec.begin(), _entryVec.end(),
					[](const AdditiveBoneEntry& a_entry)
					{
						return a_entry.channel == Engine::Resource::EAdditiveChannel::AimArm
							|| a_entry.channel == Engine::Resource::EAdditiveChannel::LagBody;
					});

				// 今の動き方 : チャージダッシュ中か・ブースト中か
				// (どちらもクエリに入れず、持っているときだけ引く)
				bool _isDashing = false;
				if (a_ctx.pWorld->HasComponent<ChargeDashComponent>(_self))
				{
					if (const auto* _pDash = a_ctx.pWorld->RefData<ChargeDashComponent>(_self))
					{
						_isDashing = _pDash->isDashing;
					}
				}
				bool _isBoost = false;
				if (a_ctx.pWorld->HasComponent<BoostStateComponent>(_self))
				{
					if (const auto* _pBoost = a_ctx.pWorld->RefData<BoostStateComponent>(_self))
					{
						_isBoost = _pBoost->isBoosting || (_pBoost->tapBoostTimer > 0.0f);
					}
				}

				// 接地していない間を空中とする。チャージダッシュは地面すれすれでも
				// 体ごと倒して突っ込ませたいので、出ている間は空中扱いにする
				bool _isAir = _isDashing;
				if (_hasAirChannel && a_ctx.pWorld->HasComponent<GroundStateComponent>(_self))
				{
					if (const auto* _pGround = a_ctx.pWorld->RefData<GroundStateComponent>(_self))
					{
						_isAir |= !_pGround->isGround;
					}
				}

				if (_hasAirChannel)
				{
					const float _airTarget = _isAir ? 1.0f : 0.0f;
					const float _airStep = _addComp.airBlendRate * a_ctx.dt;
					if (_addComp.airBlend < _airTarget)	_addComp.airBlend = (std::min)(_addComp.airBlend + _airStep, _airTarget);
					else								_addComp.airBlend = (std::max)(_addComp.airBlend - _airStep, _airTarget);
				}
				else
				{
					_addComp.airBlend = 0.0f;
				}
				const float _airBlend = _addComp.airBlend;
				const float _groundBlend = 1.0f - _airBlend;

				//==========================================================================
				// 体全体の前のめり(LagBody)の角度
				//
				// 動き方ごとに倒す最大角を変える(通常 < ブースト < チャージダッシュ)。
				// 向きは水平の速度の向き : 進んでいる側へ頭が出て、脚が逆へ流れる。
				// 速度の大きさは leanFullSpeed で頭打ちにし、それ以上はその動き方の最大角のまま。
				// 地上用の Lag(lagAngle)とは別のバネで追わせる(最大角が桁違いなので)。
				//
				// X 軸まわり : 前後(前へ進むと正 = 前のめり)
				// Z 軸まわり : 左右(右へ進むと負 = 右へ倒れる)。地上の Lag と同じ符号
				//==========================================================================
				{
					const float _leanDeg =
						_isDashing ? _addComp.leanDashDeg :
						_isBoost   ? _addComp.leanBoostDeg :
						             _addComp.leanNormalDeg;

					Math::Vector3 _leanTarget = {};
					const float _horizSpeed = std::sqrt(_velModel.x * _velModel.x + _velModel.z * _velModel.z);
					if (_airBlend > 0.0f && _horizSpeed > 1e-3f)
					{
						const float _speedRate = (_addComp.leanFullSpeed > 0.0f)
							? std::clamp(_horizSpeed / _addComp.leanFullSpeed, 0.0f, 1.0f)
							: 1.0f;
						const float _leanRad = DirectX::XMConvertToRadians(_leanDeg) * _speedRate;

						_leanTarget.x = ( _velModel.z / _horizSpeed) * _leanRad;
						_leanTarget.z = (-_velModel.x / _horizSpeed) * _leanRad;
					}

					Math::Vector3 _leanAngle(_addComp.bodyLeanAngle);
					Math::Vector3 _leanVel(_addComp.bodyLeanVelocity);
					_leanVel += ((_leanTarget - _leanAngle) * _addComp.leanStiffness - _leanVel * _addComp.leanDamping) * a_ctx.dt;
					_leanAngle += _leanVel * a_ctx.dt;
					_addComp.bodyLeanAngle = _leanAngle;
					_addComp.bodyLeanVelocity = _leanVel;
				}
				const Math::Vector3 _bodyLeanAngle(_addComp.bodyLeanAngle);

				//==========================================================================
				// 各ボーンへ適用
				//==========================================================================
				// 効きが0でも上の状態更新は済ませてある(復帰時に飛ばないようにするため)
				// ステートごとの効きは今のステートのノードから引く(引けなければ効かせきる)
				float _stateWeight = 1.0f;
				if (const auto* _pAnimator = a_ctx.pServices->pResourceManager->Get(_animComp.baseLayer.animatorHandle))
				{
					if (const auto* _pNode = _pAnimator->GetStateNode(_animComp.baseLayer.currentStateHash))
					{
						_stateWeight = _pNode->additiveWeight;
					}
				}
				float _weight = std::clamp(_addComp.masterWeight * _stateWeight, 0.0f, 1.0f);
				if (_weight <= 0.0f) continue;

				// Lag 系の回転(モデル空間)。軸ごとの効きと倍率を掛けて作る
				auto _MakeLagQuat = [&_lagAngle](const AdditiveBoneEntry& a_entry, float a_scale)
					{
						Math::Vector3 _axisScale(a_entry.axisScale);
						return
							Math::Quaternion::CreateFromAxisAngle(Math::Vector3(1.0f, 0.0f, 0.0f), _lagAngle.x * _axisScale.x * a_scale) *
							Math::Quaternion::CreateFromAxisAngle(Math::Vector3(0.0f, 1.0f, 0.0f), _lagAngle.y * _axisScale.y * a_scale) *
							Math::Quaternion::CreateFromAxisAngle(Math::Vector3(0.0f, 0.0f, 1.0f), _lagAngle.z * _axisScale.z * a_scale);
					};

				//--------------------------------------------------------------------------
				// 空中用は角度をそのまま出したいので、ステートごとの効き(additiveWeight)は掛けず
				// エンティティ全体の効きだけを掛ける(腕はレティクルへ向けきる)
				//--------------------------------------------------------------------------
				const float _airWeight = std::clamp(_addComp.masterWeight, 0.0f, 1.0f) * _airBlend;

				// 前のめり(LagBody)の回転(モデル空間)。
				// 90度近くまで倒すので、X と Z を順に掛けず、1本の軸まわりの回転として作る
				// (順に掛けると斜めへ進むときに向きがねじれる)
				auto _MakeLeanQuat = [&_bodyLeanAngle](const AdditiveBoneEntry& a_entry, float a_scale)
					{
						Math::Vector3 _axisScale(a_entry.axisScale);
						Math::Vector3 _rot(
							_bodyLeanAngle.x * _axisScale.x * a_scale,
							_bodyLeanAngle.y * _axisScale.y * a_scale,
							_bodyLeanAngle.z * _axisScale.z * a_scale);
						const float _angle = _rot.Length();
						if (!(_angle > 1e-6f)) return Math::Quaternion::Identity();
						return Math::Quaternion::CreateFromAxisAngle(_rot / _angle, _angle);
					};

				//--------------------------------------------------------------------------
				// 体全体の前のめり(LagBody)の合計。
				// 腕は腰の子なので、腰を傾けると腕の狙いも一緒に傾く。
				// AimArm ではこの分を打ち消してから狙いを足す(腕の加算 → 腰の加算の順に効くので
				// 腕 = 狙い * 前のめり^-1 にしておけば、合わせて狙いの向きになる)
				//--------------------------------------------------------------------------
				Math::Quaternion _bodyLeanQuat = Math::Quaternion::Identity();
				if (_airWeight > 0.0f)
				{
					for (const AdditiveBoneEntry& _entry : _entryVec)
					{
						if (_entry.channel != Engine::Resource::EAdditiveChannel::LagBody) continue;
						_bodyLeanQuat = _bodyLeanQuat * _MakeLeanQuat(_entry, _entry.share * _airWeight);
					}
					_bodyLeanQuat.Normalize();
				}

				for (const AdditiveBoneEntry& _entry : _entryVec)
				{
					if (_entry.nodeIdx < 0) continue;
					if (static_cast<size_t>(_entry.nodeIdx) >= _nodePoseVec.size()) continue;
					if (static_cast<size_t>(_entry.nodeIdx) >= _nodeVec.size()) continue;

					// モデル空間での加算回転を作る
					Math::Quaternion _modelQuat = Math::Quaternion::Identity();

					switch (_entry.channel)
					{
					case Engine::Resource::EAdditiveChannel::Aim:
					{
						// チェーン内の配分だけ効かせる(空中では腕へ任せて止める)
						float _share = std::clamp(_entry.share * _weight * _groundBlend, 0.0f, 1.0f);
						_modelQuat = Math::Quaternion::Slerp(Math::Quaternion::Identity(), _currentAim, _share);
						break;
					}
					case Engine::Resource::EAdditiveChannel::AimArm:
					{
						// 空中だけ、腕で狙う。前のめりで傾いた分を打ち消してから狙いを足す
						float _share = std::clamp(_entry.share * _airWeight, 0.0f, 1.0f);
						if (_share <= 0.0f) continue;
						Math::Quaternion _aimPart = Math::Quaternion::Slerp(Math::Quaternion::Identity(), _currentAim, _share);
						Math::Quaternion _cancelPart = Math::Quaternion::Slerp(
							Math::Quaternion::Identity(), _bodyLeanQuat.Conjugate(), std::clamp(_entry.share, 0.0f, 1.0f));
						_modelQuat = _aimPart * _cancelPart;
						break;
					}
					case Engine::Resource::EAdditiveChannel::LagBody:
					{
						// 空中だけ、腰を支点に体全体を前のめりにする(脚が進行方向の逆へ流れる)
						float _scale = _entry.share * _airWeight;
						if (_scale == 0.0f) continue;
						_modelQuat = _MakeLeanQuat(_entry, _scale);
						break;
					}
					default:
					{
						float _channelScale =
							(_entry.channel == Engine::Resource::EAdditiveChannel::LagArm)
							? _addComp.lagArmScale
							: _addComp.lagLegScale;

						_modelQuat = _MakeLagQuat(_entry, _entry.share * _weight * _channelScale);
						break;
					}
					}
					_modelQuat.Normalize();

					// ボーン空間へ移して、クリップ適用済みのローカル行列へ前から掛ける
					Math::Quaternion _boneQuat = ToBoneSpace(_modelQuat, _nodeVec[_entry.nodeIdx].worldTransform);

					Math::Matrix _addMat = Math::Matrix::CreateFromQuaternion(_boneQuat);
					Math::Matrix _local(_nodePoseVec[_entry.nodeIdx].local);
					_nodePoseVec[_entry.nodeIdx].local = _addMat * _local;
				}
			}
		}
	)
	// 順序 : クリップ(基本レイヤー・上に重ねるレイヤー)を書き終えた後に足す。
	// 上に重ねるレイヤーより先に足すと、上書きで狙いが消える
	.After("UpperAnimationSystem")
	// 絞り込みに使わない読み : ロック相手・実速度・接地(空中の切り替え)
	.Reads<LockOnTargetComponent, ActualVelocityComponent, GroundStateComponent, BoostStateComponent, ChargeDashComponent>();
}
