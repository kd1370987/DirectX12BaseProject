#include "PlayerIntentSystem.h"
#include "Engine/ECS/World/World.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"
#include "../../../../../Engine/Resource/Manager/InstancePoolManager/InstancePoolManager.h"

void PlayerIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<MoveIntentComponent, StateMachineComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"PlayerIntentSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			MoveIntentComponent* a_moveIntentArray,
			StateMachineComponent* a_smArray
			)
		{
			// 毎フレーム計算するのは無駄なので、パラメータ名のハッシュ値はstaticで保持しておく
			static const UINT s_speedHash = StringUtility::ToHash("Speed");
			static const UINT s_jumpHash = StringUtility::ToHash("Jump");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const MoveIntentComponent& _intentComp = a_moveIntentArray[_i];
				StateMachineComponent& _smComp = a_smArray[_i];

				// インスタンスの実体を取得
				auto* _pInstanceData = Engine::Resource::InstancePoolManager::Instance().Ref(_smComp.instanceHandle);
				if (!_pInstanceData) continue;

				// 1. 移動量から「Speed」パラメータを計算して登録
				// XとZの入力値からベクトルの長さ（速さ）を求める
				float _speed = std::sqrt((_intentComp.value.x * _intentComp.value.x) +
					(_intentComp.value.z * _intentComp.value.z));
				_pInstanceData->floatParams[s_speedHash] = _speed;

				// ジャンプ入力を「Jump」パラメータ(TriggerやBool想定)に登録
				if (_intentComp.value.y > 0.0f) // Y軸にジャンプ入力が入っている場合
				{
					_pInstanceData->boolParams[s_jumpHash] = true;
				}
			}
		}
	);
}
