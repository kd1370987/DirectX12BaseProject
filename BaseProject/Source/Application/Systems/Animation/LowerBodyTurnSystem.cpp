#include "LowerBodyTurnSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Application/Components/Animation/LowerBodyTurnComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Physics/GroundStateComponent.h"
#include "Application/Components/Movement/ChargeDashComponent.h"

//==========================================================================================
// LowerBodyTurnSystem
//
// 機体(= 上半身)の向きと脚の向きの差の分だけ、腰(pivot)をモデル空間の Y 軸まわりに
// ひねり、上半身の根元(counter)を同じだけ逆にひねって打ち消す。
//
// ・行ベクトル規約なので world = local * parentWorld。
//   腰のワールド W を、腰の位置 t を中心に Rm だけ回したいので
//     W' = W * K      (K = T(-t) * Rm * T(t))
//     L' = W' * P^-1  (P は腰の親のワールド)
//   上半身の根元は W' の子になっても見た目が変わらないように
//     S' * W' = S * W  →  S' = S * W * K^-1 * W^-1
// ・ワールド行列はまだ組まれていない(CalcNodeSystem が後で組む)ので、
//   親のワールドはこのフレームのローカルを根元まで掛けて求める。
//==========================================================================================
namespace
{
	// -π〜π に丸める
	float WrapAngle(float a_rad)
	{
		constexpr float PI = DirectX::XM_PI;
		constexpr float TWO_PI = DirectX::XM_2PI;
		a_rad = std::fmod(a_rad + PI, TWO_PI);
		if (a_rad < 0.0f) a_rad += TWO_PI;
		return a_rad - PI;
	}

	// 名前のハッシュからノード番号を引く。無ければ -1
	int FindNodeIndex(const std::vector<Engine::Resource::Node>& a_nodeVec, UINT a_nameHash)
	{
		if (a_nameHash == 0) return -1;
		for (size_t _i = 0; _i < a_nodeVec.size(); ++_i)
		{
			if (a_nodeVec[_i].nodeNameHash == a_nameHash) return static_cast<int>(_i);
		}
		return -1;
	}

	// このフレームのローカルを根元まで掛けて、ノードのワールド(モデル空間)を求める
	Math::Matrix CalcWorld(const std::vector<Engine::Resource::Node>& a_nodeVec, std::span<Engine::Resource::NodePoseMatrix> a_poseVec, int a_nodeIdx)
	{
		Math::Matrix _world = Math::Matrix::Identity();
		for (int _idx = a_nodeIdx; _idx >= 0; _idx = a_nodeVec[_idx].parent)
		{
			_world = _world * a_poseVec[_idx].local;
		}
		return _world;
	}

	// 水平方向のベクトルから Yaw を作る(左手系で +Z が前方なので atan2(x, z))
	bool CalcYawFromDir(const Math::Vector3& a_dir, float a_minLength, float& a_outYaw)
	{
		const float _lenSq = a_dir.x * a_dir.x + a_dir.z * a_dir.z;
		// NaN も弾ける形で書く
		if (!(_lenSq > a_minLength * a_minLength) || !(_lenSq > 1e-8f)) return false;

		a_outYaw = std::atan2(a_dir.x, a_dir.z);
		return true;
	}
}

void LowerBodyTurnSystem::Init(App::ECS::APPWorld& a_world)
{
	// 自分のチャンクとプールの自分の範囲だけを書くので、チャンクを分けてワーカーで回す
	a_world.ActiveJobTask<
		const ModelComponent,
		const LocalTransformComponent,
		const DesiredVelocityComponent,
		LowerBodyTurnComponent,
		NodePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"LowerBodyTurnSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			const LocalTransformComponent* a_trsArray,
			const DesiredVelocityComponent* a_velArray,
			LowerBodyTurnComponent* a_turnArray,
			NodePoseComponent* a_nodePoseArray
		)
		{
			auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LocalTransformComponent& _trs = a_trsArray[_i];
				LowerBodyTurnComponent& _turnComp = a_turnArray[_i];

				//==================================================================
				// 脚の向き(ワールドの Yaw)を進める
				//==================================================================
				// 機体の向き
				const Math::Vector3 _bodyForward = Math::Matrix::CreateFromQuaternion(_trs.quat).Forward();
				float _bodyYaw = 0.0f;
				if (!CalcYawFromDir(_bodyForward, 0.0f, _bodyYaw)) continue;

				if (!_turnComp.isLegYawValid)
				{
					_turnComp.legYaw = _bodyYaw;
					_turnComp.isLegYawValid = true;
				}

				// 接地していない間とチャージダッシュ中は進行方向を追わず、機体の正面(ひねり無し)へ戻す。
				// (加算ポーズもチャージダッシュ中は空中扱いで体ごと倒す)
				// 接地判定を持たないものは地上として扱う
				bool _isAir = false;
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (a_ctx.pWorld->HasComponent<ChargeDashComponent>(_self))
				{
					if (const auto* _pDash = a_ctx.pWorld->RefData<ChargeDashComponent>(_self))
					{
						_isAir = _pDash->isDashing;
					}
				}
				if (a_ctx.pWorld->HasComponent<GroundStateComponent>(_self))
				{
					if (const auto* _pGround = a_ctx.pWorld->RefData<GroundStateComponent>(_self))
					{
						_isAir |= !_pGround->isGround;
					}
				}

				// 地上では進んでいる間だけ進行方向へ寄せる。止まっていれば今の向きのまま残す
				float _targetYaw = 0.0f;
				bool _hasTarget = false;
				if (_isAir)
				{
					_targetYaw = _bodyYaw;
					_hasTarget = true;
				}
				else
				{
					_hasTarget = CalcYawFromDir(a_velArray[_i].value, _turnComp.minMoveSpeed, _targetYaw);
				}

				if (_hasTarget)
				{
					const float _t = std::clamp(_turnComp.turnRate * a_ctx.dt, 0.0f, 1.0f);
					_turnComp.legYaw = WrapAngle(_turnComp.legYaw + WrapAngle(_targetYaw - _turnComp.legYaw) * _t);
				}

				// 機体から見た脚のひねり
				const float _deltaYaw = WrapAngle(_turnComp.legYaw - _bodyYaw);
				if (std::abs(_deltaYaw) < 1e-4f) continue;

				//==================================================================
				// ポーズへ適用
				//==================================================================
				const auto* _pModel = a_ctx.pServices->pResourceManager->Get(a_modelArray[_i].handle);
				if (!_pModel) continue;

				auto _poseVec = _nodePosePool.RefRange(a_nodePoseArray[_i].nodePoseHandle);
				if (_poseVec.empty()) continue;

				const auto& _nodeVec = _pModel->GetOriginalNodeVec();
				if (_poseVec.size() < _nodeVec.size()) continue;

				const int _pivotIdx = FindNodeIndex(_nodeVec, _turnComp.pivotNodeHash);
				if (_pivotIdx < 0) continue;

				// 腰のワールドと、その親のワールド
				const int _parentIdx = _nodeVec[_pivotIdx].parent;
				const Math::Matrix _parentWorld = (_parentIdx >= 0) ? CalcWorld(_nodeVec, _poseVec, _parentIdx) : Math::Matrix::Identity();
				const Math::Matrix _pivotLocal = _poseVec[_pivotIdx].local;
				const Math::Matrix _pivotWorld = _pivotLocal * _parentWorld;

				float _det = 0.0f;
				const Math::Matrix _invParentWorld = _parentWorld.Invert(_det);
				if (!std::isfinite(_det) || _det == 0.0f) continue;

				const Math::Matrix _invPivotWorld = _pivotWorld.Invert(_det);
				if (!std::isfinite(_det) || _det == 0.0f) continue;

				// 腰の位置を中心に、モデル空間の Y 軸まわりに回す
				const Math::Vector3 _pivotPos = _pivotWorld.Translation();
				const Math::Matrix _toPivot = Math::Matrix::CreateTranslation(-_pivotPos);
				const Math::Matrix _fromPivot = Math::Matrix::CreateTranslation(_pivotPos);
				const Math::Matrix _turn = _toPivot * Math::Matrix::CreateFromYawPitchRoll(_deltaYaw, 0.0f, 0.0f) * _fromPivot;
				const Math::Matrix _turnInv = _toPivot * Math::Matrix::CreateFromYawPitchRoll(-_deltaYaw, 0.0f, 0.0f) * _fromPivot;

				// 腰から下をひねる
				_poseVec[_pivotIdx].local = _pivotWorld * _turn * _invParentWorld;

				// 上半身の根元は逆にひねって、機体の向きのまま残す(腰の直下にあるときだけ)
				const int _counterIdx = FindNodeIndex(_nodeVec, _turnComp.counterNodeHash);
				if (_counterIdx >= 0 && _nodeVec[_counterIdx].parent == _pivotIdx)
				{
					_poseVec[_counterIdx].local = _poseVec[_counterIdx].local * _pivotWorld * _turnInv * _invPivotWorld;
				}
			}
		}
	)
	// 順序 : クリップ(基本・上に重ねるレイヤー)と加算ポーズを書き終えた後、ワールドを組む前
	.After("AdditivePoseSystem")
	.Before("CalcNodeSystem")
	// 絞り込みに使わない読み : 接地・チャージダッシュ(空中では脚を正面へ戻す)
	.Reads<GroundStateComponent, ChargeDashComponent>();
}
