#include "AdditivePoseSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Character/Robot/AdditivePoseComponent.h"
#include "Application/Components/Character/LookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/InertiaComponent.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

namespace
{
	//----------------------------------------------------------------------------------
	// 回転行列の前方ベクトルから Yaw / Pitch(ラジアン)を取り出す。
	// このエンジンは行ベクトル規約なので、前方ベクトルは行列の第3行になる。
	//----------------------------------------------------------------------------------
	void ExtractYawPitch(const DXSM::Quaternion& a_quat, float& a_outYaw, float& a_outPitch)
	{
		DXSM::Matrix _mat = DXSM::Matrix::CreateFromQuaternion(a_quat);
		DXSM::Vector3 _forward(_mat._31, _mat._32, _mat._33);
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
	DXSM::Quaternion ToBoneSpace(const DXSM::Quaternion& a_modelQuat, const DirectX::XMFLOAT4X4& a_bindWorld)
	{
		DXSM::Matrix _bindMat(a_bindWorld);

		DXSM::Vector3 _bindScale;
		DXSM::Quaternion _bindRot;
		DXSM::Vector3 _bindTrans;
		if (!_bindMat.Decompose(_bindScale, _bindRot, _bindTrans))
		{
			// バインド行列が壊れている場合はモデル空間のまま適用する
			return a_modelQuat;
		}

		DXSM::Quaternion _bindConj;
		_bindRot.Conjugate(_bindConj);

		DXSM::Quaternion _result = _bindRot * a_modelQuat * _bindConj;
		_result.Normalize();
		return _result;
	}
}

void AdditivePoseSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<
		const ModelComponent,
		const AnimatorComponent,
		const LookAngleComponent,
		const LocalTransformComponent,
		const VelocityComponent,
		NodePoseComponent,
		AdditivePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"AdditivePoseSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			const AnimatorComponent* a_animatorArray,
			const LookAngleComponent* a_lookArray,
			const LocalTransformComponent* a_trsArray,
			const VelocityComponent* a_velocityArray,
			NodePoseComponent* a_nodePoseArray,
			AdditivePoseComponent* a_additiveArray
		)
		{
			if (a_ctx.dt <= 0.0f) return;

			auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
			auto& _entryPool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				const AnimatorComponent& _animComp = a_animatorArray[_i];
				const LookAngleComponent& _lookComp = a_lookArray[_i];
				const LocalTransformComponent& _trsComp = a_trsArray[_i];
				const VelocityComponent& _velComp = a_velocityArray[_i];
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
				DXSM::Quaternion _bodyQuat(_trsComp.quat);
				if (_bodyQuat.LengthSquared() < 1e-6f) _bodyQuat = DXSM::Quaternion::Identity;
				_bodyQuat.Normalize();

				DXSM::Quaternion _bodyConj = _bodyQuat;
				_bodyConj.Conjugate();

				//==========================================================================
				// Aim: 照準方向と機体の向きの差分を、モデル空間の回転として求める
				//--------------------------------------------------------------------------
				// 角度は度で保持されているのでラジアンへ変換する。
				// Vector3 オーバーロードは(pitch,yaw,roll)順で軸が入れ替わるため、
				// スカラー版 CreateFromYawPitchRoll(yaw,pitch,roll) を明示的に使う。
				//==========================================================================
				DXSM::Quaternion _lookQuat = DXSM::Quaternion::CreateFromYawPitchRoll(
					DirectX::XMConvertToRadians(_lookComp.Yaw),
					DirectX::XMConvertToRadians(-_lookComp.Pitch),
					0.0f
				);
				_lookQuat.Normalize();

				// Mat(offset) = Mat(look) * Mat(body)^-1 となる差分
				DXSM::Quaternion _offsetQuat = _lookQuat * _bodyConj;
				_offsetQuat.Normalize();

				// 可動域でクランプしてから作り直す(ロールは捨てる)
				float _yaw = 0.0f;
				float _pitch = 0.0f;
				ExtractYawPitch(_offsetQuat, _yaw, _pitch);

				const float _yawLimit = DirectX::XMConvertToRadians(_addComp.yawLimitDeg);
				const float _pitchLimit = DirectX::XMConvertToRadians(_addComp.pitchLimitDeg);
				_yaw = std::clamp(_yaw, -_yawLimit, _yawLimit);
				_pitch = std::clamp(_pitch, -_pitchLimit, _pitchLimit);

				DXSM::Quaternion _targetAim = DXSM::Quaternion::CreateFromYawPitchRoll(_yaw, _pitch, 0.0f);
				_targetAim.Normalize();

				// Slerpで追従(急に振り向かせない)
				DXSM::Quaternion _currentAim(_addComp.currentAimQuat);
				if (_currentAim.LengthSquared() < 1e-6f) _currentAim = DXSM::Quaternion::Identity;

				float _t = std::min(_addComp.followRate * a_ctx.dt, 1.0f);
				_currentAim = DXSM::Quaternion::Slerp(_currentAim, _targetAim, _t);
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
				// 慣性を持つ機体は InertiaComponent の実速度を見る。
				// VelocityComponent は「目標速度」で、入力やブーストで 0 → 30 のように
				// 1フレームで飛ぶ値なので、そのまま使うと流れ始めが階段状になる。
				// 実速度は慣性で滑らかに変化するので、歩き出しは浅く、
				// ブーストは深く、と速さがそのまま角度に出る。
				//==========================================================================
				DXSM::Vector3 _velocity(_velComp.value);

				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (a_ctx.pWorld->HasComponent<InertiaComponent>(_self))
				{
					if (const auto* _pInertia = a_ctx.pWorld->RefData<InertiaComponent>(_self))
					{
						_velocity = DXSM::Vector3(_pInertia->velocity);
					}
				}

				// ワールド速度をモデル空間へ(x:右 y:上 z:前)
				DXSM::Vector3 _velModel = DXSM::Vector3::Transform(_velocity, _bodyConj);

				// 前方向の速度は X軸(右)まわり、横方向の速度は Z軸(前)まわりに倒す。
				// どちらも軸スケールが正のときに「進行方向の逆へ流れる」向きになる
				// (下向きの手足に対して、+X 回転は後ろへ、+Z 回転は右へ振れるため符号が入れ替わる)
				const float _lagLimit = DirectX::XMConvertToRadians(_addComp.lagLimitDeg);
				DXSM::Vector3 _lagTarget = {};
				_lagTarget.x = std::clamp(_velModel.z * _addComp.lagScale, -_lagLimit, _lagLimit);
				_lagTarget.y = 0.0f;
				_lagTarget.z = std::clamp(-_velModel.x * _addComp.lagScale, -_lagLimit, _lagLimit);

				// バネで追従させ、加速が止まったら自然に戻す
				DXSM::Vector3 _lagAngle(_addComp.lagAngle);
				DXSM::Vector3 _lagVel(_addComp.lagVelocity);

				_lagVel += ((_lagTarget - _lagAngle) * _addComp.lagStiffness - _lagVel * _addComp.lagDamping) * a_ctx.dt;
				_lagAngle += _lagVel * a_ctx.dt;

				_lagAngle.x = std::clamp(_lagAngle.x, -_lagLimit, _lagLimit);
				_lagAngle.y = std::clamp(_lagAngle.y, -_lagLimit, _lagLimit);
				_lagAngle.z = std::clamp(_lagAngle.z, -_lagLimit, _lagLimit);

				_addComp.lagAngle = _lagAngle;
				_addComp.lagVelocity = _lagVel;

				//==========================================================================
				// 各ボーンへ適用
				//==========================================================================
				// 効きが0でも上の状態更新は済ませてある(復帰時に飛ばないようにするため)
				float _weight = std::clamp(_addComp.masterWeight * _animComp.additiveWeight, 0.0f, 1.0f);
				if (_weight <= 0.0f) continue;

				for (const AdditiveBoneEntry& _entry : _entryVec)
				{
					if (_entry.nodeIdx < 0) continue;
					if (static_cast<size_t>(_entry.nodeIdx) >= _nodePoseVec.size()) continue;
					if (static_cast<size_t>(_entry.nodeIdx) >= _nodeVec.size()) continue;

					// モデル空間での加算回転を作る
					DXSM::Quaternion _modelQuat = DXSM::Quaternion::Identity;

					if (_entry.channel == Engine::Resource::EAdditiveChannel::Aim)
					{
						// チェーン内の配分だけ効かせる
						float _share = std::clamp(_entry.share * _weight, 0.0f, 1.0f);
						_modelQuat = DXSM::Quaternion::Slerp(DXSM::Quaternion::Identity, _currentAim, _share);
					}
					else
					{
						float _channelScale =
							(_entry.channel == Engine::Resource::EAdditiveChannel::LagArm)
							? _addComp.lagArmScale
							: _addComp.lagLegScale;

						float _scale = _entry.share * _weight * _channelScale;

						DXSM::Vector3 _axisScale(_entry.axisScale);
						_modelQuat =
							DXSM::Quaternion::CreateFromAxisAngle(DXSM::Vector3(1.0f, 0.0f, 0.0f), _lagAngle.x * _axisScale.x * _scale) *
							DXSM::Quaternion::CreateFromAxisAngle(DXSM::Vector3(0.0f, 1.0f, 0.0f), _lagAngle.y * _axisScale.y * _scale) *
							DXSM::Quaternion::CreateFromAxisAngle(DXSM::Vector3(0.0f, 0.0f, 1.0f), _lagAngle.z * _axisScale.z * _scale);
					}
					_modelQuat.Normalize();

					// ボーン空間へ移して、クリップ適用済みのローカル行列へ前から掛ける
					DXSM::Quaternion _boneQuat = ToBoneSpace(_modelQuat, _nodeVec[_entry.nodeIdx].worldTransform);

					DXSM::Matrix _addMat = DXSM::Matrix::CreateFromQuaternion(_boneQuat);
					DXSM::Matrix _local(_nodePoseVec[_entry.nodeIdx].local);
					_nodePoseVec[_entry.nodeIdx].local = _addMat * _local;
				}
			}
		}
	);
}
