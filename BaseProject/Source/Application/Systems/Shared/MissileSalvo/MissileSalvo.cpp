#include "MissileSalvo.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"
#include "Engine/Resource/Data/Model/Model.h"

#include "Application/Components/Character/Weapon/Gun/GunStateComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "../ProjectileSpawn/ProjectileSpawn.h"

//==========================================================================================
// MissileSalvo
//
// 一斉射の「撃つ」ほうだけを持つ。誰を狙うか(溜め)は呼び出し側の担当。
//==========================================================================================
namespace App::Systems::MissileSalvo
{
	Math::Vector3 MakeSpreadDir(
		const Math::Vector3& a_baseDir,
		float                a_spreadRad,
		int                  a_index,
		int                  a_total)
	{
		if (a_spreadRad <= 0.0f || a_total <= 0) return a_baseDir;

		// 基準方向に直交する基底を作る。真上を向いている時だけ参照軸を変える
		const Math::Vector3 _ref = (std::fabs(a_baseDir.y) > 0.99f)
			? Math::Vector3(1.0f, 0.0f, 0.0f)
			: Math::Vector3(0.0f, 1.0f, 0.0f);

		Math::Vector3 _right = _ref.Cross(a_baseDir);
		if (_right.LengthSquared() <= 1e-8f) return a_baseDir;
		_right.Normalize();

		Math::Vector3 _up = a_baseDir.Cross(_right);
		_up.Normalize();

		// コーンの周りを等間隔に回す
		const float _azimuth = DirectX::XM_2PI * static_cast<float>(a_index) / static_cast<float>(a_total);

		Math::Vector3 _dir =
			a_baseDir * std::cos(a_spreadRad) +
			(_right * std::cos(_azimuth) + _up * std::sin(_azimuth)) * std::sin(a_spreadRad);

		_dir.Normalize();
		return _dir;
	}

	void ConsumeFireQueue(
		const Engine::ECS::SystemContext& a_ctx,
		MissileLockComponent&             a_missile,
		Engine::ECS::Entity               a_podEntity,
		const Math::Vector3&              a_aimDir)
	{
		if (!a_missile.IsFiring()) return;
		if (!a_ctx.pWorld || !a_ctx.pServices || !a_ctx.pServices->pResourceManager) return;

		a_missile.fireTimer -= a_ctx.dt;
		if (a_missile.fireTimer > 0.0f) return;

		//------------------------------------------------------------------
		// ミサイルポッド(発射する武器)を引く
		//------------------------------------------------------------------
		if (a_podEntity == Engine::ECS::Limits::INVALID_ENTITY ||
			!a_ctx.pWorld->HasComponent<GunStateComponent>(a_podEntity) ||
			!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(a_podEntity))
		{
			// 武器が付いていないなら撃てない。キューは捨てる
			a_missile.fireRemain = 0;
			return;
		}

		auto* _pGun      = a_ctx.pWorld->RefData<GunStateComponent>(a_podEntity);
		auto* _pPodWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(a_podEntity);
		if (!_pGun || !_pPodWorld)
		{
			a_missile.fireRemain = 0;
			return;
		}

		// プレハブ未設定なら撃てない
		if (_pGun->bulletPrefabGUID == Engine::DefaultGUID)
		{
			a_missile.fireRemain = 0;
			return;
		}

		// プレハブのハンドルを解決(未ロードならロード)
		auto& _rm = *a_ctx.pServices->pResourceManager;
		if (!_rm.IsValid(_pGun->bulletPrefabHandle))
		{
			// 取った参照は銃が消えるときに返す
			// (GunStateComponent の解放フック)
			_rm.AcquireImmediate(_pGun->bulletPrefabHandle, _pGun->bulletPrefabGUID);
		}
		auto* _pPrefab = _rm.Ref(_pGun->bulletPrefabHandle);
		if (!_pPrefab) return;

		//------------------------------------------------------------------
		// 発射位置 : 銃口ヌルノードが設定されていればそこから
		// (node.worldTransform はモデルルート基準なので、ポッドのワールド行列で
		//  変換してワールド座標にする。GunShootSystem と同じ)
		//------------------------------------------------------------------
		const Math::Matrix& _podMat = _pPodWorld->worldMat;
		Math::Vector3 _spawnPos = { _podMat._41, _podMat._42, _podMat._43 };

		if (_pGun->nullPtrNodeHash != 0 &&
			a_ctx.pWorld->HasComponent<ModelComponent>(a_podEntity))
		{
			if (const auto* _pModelComp = a_ctx.pWorld->RefData<ModelComponent>(a_podEntity))
			{
				auto* _pModel = a_ctx.pServices->pResourceManager->Get(_pModelComp->handle);
				if (_pModel)
				{
					const auto& _nodeVec = _pModel->GetOriginalNodeVec();
					if (_pGun->nodeIndex < _nodeVec.size())
					{
						const Math::Matrix& _nodeMat = _nodeVec[_pGun->nodeIndex].worldTransform;
						Math::Vector3 _nodeLocalPos = { _nodeMat._41, _nodeMat._42, _nodeMat._43 };
						_spawnPos = Math::Vector3::Transform(_nodeLocalPos, Math::Matrix(_podMat));
					}
				}
			}
		}

		// 狙いの向きが潰れていたらポッドの前方で代用する
		Math::Vector3 _aimDir = a_aimDir;
		if (_aimDir.LengthSquared() > 1e-8f) _aimDir.Normalize();
		else                                 _aimDir = Math::Vector3(_podMat._31, _podMat._32, _podMat._33);
		if (_aimDir.LengthSquared() > 1e-8f) _aimDir.Normalize();
		else                                 _aimDir = Math::Vector3(0.0f, 0.0f, 1.0f);

		const float _spreadRad = DirectX::XMConvertToRadians(std::max(a_missile.spreadAngle, 0.0f));
		const Engine::ECS::Entity _shooter =
			App::Systems::ProjectileSpawn::ResolveShooterEntity(*a_ctx.pWorld, a_podEntity);

		//------------------------------------------------------------------
		// 溜まっているぶんを撃つ
		//   launchInterval が 0 なら timer が進まないので同フレームに全弾出る
		//------------------------------------------------------------------
		while (a_missile.fireRemain > 0 && a_missile.fireTimer <= 0.0f)
		{
			// 何発目か(散らしの角度を弾ごとにずらすため)
			const int _index = a_missile.fireTotal - a_missile.fireRemain;

			const Engine::ECS::Entity _target = a_missile.fireTargets[_index];

			// 相手が居るならそちらを基準に、居なければ狙いの向きを基準に散らす
			Math::Vector3 _baseDir = _aimDir;
			if (_target != Engine::ECS::Limits::INVALID_ENTITY &&
				a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target))
			{
				if (const auto* _pTargetWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(_target))
				{
					Math::Vector3 _targetPos = Math::Matrix(_pTargetWorld->worldMat).Translation();
					_targetPos.y += a_missile.targetOffsetY;

					Math::Vector3 _toTarget = _targetPos - _spawnPos;
					if (_toTarget.LengthSquared() > 1e-6f)
					{
						_toTarget.Normalize();
						_baseDir = _toTarget;
					}
				}
			}

			const Math::Vector3 _shootDir =
				MakeSpreadDir(_baseDir, _spreadRad, _index, a_missile.fireTotal);

			App::Systems::ProjectileSpawn::Spawn(
				*a_ctx.pWorld,
				_pPrefab,
				_spawnPos,
				_shootDir * _pGun->speed,
				_shooter,
				_target);

			--a_missile.fireRemain;
			a_missile.fireTimer += std::max(a_missile.launchInterval, 0.0f);
		}
	}
}
