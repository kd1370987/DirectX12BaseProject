#include "PlayerIntentSystem.h"
#include "Engine/ECS/World/World.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"

void PlayerIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const MoveIntentComponent, const BoostComponent,StateMachineComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"PlayerIntentSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const MoveIntentComponent* a_moveIntentArray,
			const BoostComponent* a_boostComp,
			StateMachineComponent* a_smArray
			)
		{
			// 毎フレーム計算するのは無駄なので、パラメータ名のハッシュ値はstaticで保持しておく
			static const UINT s_speedHash = StringUtility::ToHash("Speed");
			static const UINT s_jumpHash = StringUtility::ToHash("Jump");
			static const UINT s_isGroundHash = StringUtility::ToHash("IsGround");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const MoveIntentComponent& _intentComp = a_moveIntentArray[_i];
				const BoostComponent& _boostComp = a_boostComp[_i];
				StateMachineComponent& _smComp = a_smArray[_i];

				// インスタンスの実体を取得
				auto& _stateInstancePool = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
				auto* _pInstance = _stateInstancePool.Ref(_smComp.instanceHandle);
				if (!_pInstance) continue;

				// 移動量から「Speed」パラメータを計算して登録
				// XとZの入力値からベクトルの長さ（速さ）を求める
				float _speed = std::sqrt((_intentComp.value.x * _intentComp.value.x) +
					(_intentComp.value.z * _intentComp.value.z));
				_pInstance->floatParams[s_speedHash] = _speed;

				// ジャンプ入力を「Jump」パラメータ(TriggerやBool想定)に登録
				if (_intentComp.value.y > 0.0f) // Y軸にジャンプ入力が入っている場合
				{
					_pInstance->boolParams[s_jumpHash] = true;
				}
				else
				{
					_pInstance->boolParams[s_jumpHash] = false;
				}

				// 地面に接しているかの判定
				_pInstance->boolParams[s_isGroundHash] = _smComp.isGround;
			}
		}
	);
}
