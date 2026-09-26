#include "PlayerIntentSystem.h"
#include "Application/ECS/World/APPWorld.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../Components/Collision/GroundStateComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

//==========================================================================================
// PlayerIntentSystem
//
// 入力・状態から、アニメーター(AnimatorAsset)のパラメータを毎フレーム更新する。
//
// パラメータは AnimatorAsset::SetXxxParam 経由で書き込む。
// これは「設計図の定義に無ければ定義を追加してから値を入れる」ので、
// プログラム側から足したパラメータもそのままエディターの一覧に出る。
// (定義済みならエディターで設定した型/デフォルト値をそのまま使う)
//
// StateMachineComponent はハンドルを読むだけなので const。
// 値を書き込むのはハンドルの先のインスタンス(プール)で、コンポーネント自体は触らない
//==========================================================================================
void PlayerIntentSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const MoveIntentComponent, const BoostComponent, const StateMachineComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"PlayerIntentSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const MoveIntentComponent* a_moveIntentArray,
			const BoostComponent* a_boostComp,
			const StateMachineComponent* a_smArray
			)
		{
			// 毎フレーム計算するのは無駄なので、パラメータ名のハッシュ値はstaticで保持しておく
			static const UINT s_speedHash = Engine::String::ToHash("Speed");
			static const UINT s_jumpHash = Engine::String::ToHash("Jump");
			static const UINT s_isGroundHash = Engine::String::ToHash("IsGround");
			static const UINT s_isShootHash = Engine::String::ToHash("IsShoot");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const MoveIntentComponent& _intentComp = a_moveIntentArray[_i];
				const BoostComponent& _boostComp = a_boostComp[_i];
				const StateMachineComponent& _smComp = a_smArray[_i];
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// インスタンスの実体を取得
				auto& _stateInstancePool = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
				auto* _pInstance = _stateInstancePool.Ref(_smComp.instanceHandle);
				if (!_pInstance) continue;

				// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
				auto* _pAnimator = a_ctx.pServices->pResourceManager->Ref(_smComp.stateMachineHandle);
				if (!_pAnimator) continue;

				// 移動量から「Speed」パラメータを計算して登録
				// XとZの入力値からベクトルの長さ（速さ）を求める
				float _speed = std::sqrt((_intentComp.value.x * _intentComp.value.x) +
					(_intentComp.value.z * _intentComp.value.z));
				_pAnimator->SetFloatParam(*_pInstance, s_speedHash, "Speed", _speed);

				// ジャンプ入力を「Jump」パラメータ(TriggerやBool想定)に登録
				// Y軸にジャンプ入力が入っている場合は true
				_pAnimator->SetBoolParam(*_pInstance, s_jumpHash, "Jump", _intentComp.value.y > 0.0f);

				//--------------------------------------------------------------------------
				// 地面に接しているかの判定
				//
				// 接地判定(GroundStateComponent)は足元のレイを持つものにしか付かないので、
				// アーキタイプを狭めないようにエンティティ単位で参照する。
				// 持っていなければ接地していない扱い(以前の StateMachineComponent::isGround の既定値と同じ)
				//--------------------------------------------------------------------------
				bool _isGround = false;
				if (a_ctx.pWorld->HasComponent<GroundStateComponent>(_self))
				{
					if (const auto* _pGround = a_ctx.pWorld->RefData<GroundStateComponent>(_self))
					{
						_isGround = _pGround->isGround;
					}
				}
				_pAnimator->SetBoolParam(*_pInstance, s_isGroundHash, "IsGround", _isGround);

				//--------------------------------------------------------------------------
				// 銃関係(発射中)
				//
				// ActionIntentComponent は付いていないキャラもいるので、
				// アーキタイプを狭めないようにエンティティ単位で参照する。
				// 左右どちらかを撃っていれば「撃っている」とする。
				//--------------------------------------------------------------------------
				if (a_ctx.pWorld->HasComponent<ActionIntentComponent>(_self))
				{
					if (const auto* _pActionIntent = a_ctx.pWorld->RefData<ActionIntentComponent>(_self))
					{
						_pAnimator->SetBoolParam(*_pInstance, s_isShootHash, "IsShoot", _pActionIntent->IsAnyWeaponShoot());
					}
				}
			}
		}
	);
}
