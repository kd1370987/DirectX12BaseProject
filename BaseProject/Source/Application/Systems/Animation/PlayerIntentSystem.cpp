#include "PlayerIntentSystem.h"
#include "Application/ECS/World/APPWorld.h"
#include "Application/Components/Movement/MoveIntentComponent.h"
#include "Application/Components/Combat/ActionIntentComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/UpperAnimatorComponent.h"
#include "Application/Components/Movement/BoostParamsComponent.h"
#include "Application/Components/Movement/BoostStateComponent.h"
#include "Application/Components/Movement/ChargeDashComponent.h"
#include "Application/Components/Physics/GroundStateComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

namespace App::System
{
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
	// 上に重ねるレイヤー(UpperAnimatorComponent)を持っていれば、そちらのインスタンスにも同じ値を書く。
	// (レイヤーごとに設計図もパラメータの実体も別なので、両方へ書かないと上のレイヤーが遷移しない)
	//
	// AnimatorComponent はハンドルを読むだけなので const。
	// 値を書き込むのはハンドルの先のインスタンス(プール)で、コンポーネント自体は触らない
	//==========================================================================================
	void PlayerIntentSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<const Component::MoveIntentComponent, const Component::BoostParamsComponent, const Component::AnimatorComponent>(
			Engine::ECS::ESystemType::PreUpdate,
			"PlayerIntentSystem",
			[]
			(
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_tags*/,
				const Component::MoveIntentComponent* a_moveIntentArray,
				const Component::BoostParamsComponent* /*a_boostComp*/,
				const Component::AnimatorComponent* a_animatorArray
				)
			{
				// 毎フレーム計算するのは無駄なので、パラメータ名のハッシュ値はstaticで保持しておく
				static const UINT SPEED_HASH = Core::String::ToHash("Speed");
				static const UINT JUMP_HASH = Core::String::ToHash("Jump");
				static const UINT IS_GROUND_HASH = Core::String::ToHash("IsGround");
				static const UINT IS_SHOOT_HASH = Core::String::ToHash("IsShoot");
				static const UINT IS_BOOST_HASH = Core::String::ToHash("IsBoost");

				auto& _stateInstancePool = a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::MoveIntentComponent& _intentComp = a_moveIntentArray[_i];
					const Component::AnimatorComponent& _animComp = a_animatorArray[_i];
					const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

					// 入力・状態から値を集める
					// 移動量から「Speed」: XとZの入力値からベクトルの長さ（速さ）を求める
					const float _speed = std::sqrt((_intentComp.value.x * _intentComp.value.x) +
						(_intentComp.value.z * _intentComp.value.z));

					// ジャンプ入力(Y軸にジャンプ入力が入っている場合は true)
					const bool _isJump = _intentComp.value.y > 0.0f;

					//--------------------------------------------------------------------------
					// 地面に接しているかの判定
					//
					// 接地判定(GroundStateComponent)は足元のレイを持つものにしか付かないので、
					// アーキタイプを狭めないようにエンティティ単位で参照する。
					// 持っていなければ接地していない扱い(以前の StateMachineComponent::isGround の既定値と同じ)
					//--------------------------------------------------------------------------
					bool _isGround = false;
					if (a_ctx.pWorld->HasComponent<Component::GroundStateComponent>(_self))
					{
						if (const auto* _pGround = a_ctx.pWorld->RefData<Component::GroundStateComponent>(_self))
						{
							_isGround = _pGround->isGround;
						}
					}

					//--------------------------------------------------------------------------
					// 銃関係(発射中)
					//
					// ActionIntentComponent は付いていないキャラもいるので、
					// アーキタイプを狭めないようにエンティティ単位で参照する。
					// 左右どちらかを撃っていれば「撃っている」とする。
					//--------------------------------------------------------------------------
					const Component::ActionIntentComponent* _pActionIntent = nullptr;
					if (a_ctx.pWorld->HasComponent<Component::ActionIntentComponent>(_self))
					{
						_pActionIntent = a_ctx.pWorld->RefData<Component::ActionIntentComponent>(_self);
					}

					//--------------------------------------------------------------------------
					// ブースト中か(押し続けている間と、踏み込みの初動が残っている間。チャージダッシュ中も含める)
					//
					// 状態(BoostStateComponent)は BoostParamsComponent の必須コンポーネントなので
					// 必ず付いているが、絞り込みの配列には入れていないのでエンティティ単位で引く
					//--------------------------------------------------------------------------
					bool _isBoost = false;
					if (const auto* _pBoostState = a_ctx.pWorld->RefData<Component::BoostStateComponent>(_self))
					{
						_isBoost = _pBoostState->isBoosting || (_pBoostState->tapBoostTimer > 0.0f);
					}
					if (a_ctx.pWorld->HasComponent<Component::ChargeDashComponent>(_self))
					{
						if (const auto* _pDash = a_ctx.pWorld->RefData<Component::ChargeDashComponent>(_self))
						{
							_isBoost |= _pDash->isDashing;
						}
					}

					// レイヤー1枚ぶんのインスタンスへ書く
					auto _WriteParams = [&](const Component::AnimatorLayer& a_layer)
						{
							// インスタンスの実体を取得
							auto* _pInstance = _stateInstancePool.Ref(a_layer.instanceHandle);
							if (!_pInstance) return;

							// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
							auto* _pAnimator = a_ctx.pServices->pResourceManager->Ref(a_layer.animatorHandle);
							if (!_pAnimator) return;

							_pAnimator->SetFloatParam(*_pInstance, SPEED_HASH, "Speed", _speed);
							_pAnimator->SetBoolParam(*_pInstance, JUMP_HASH, "Jump", _isJump);
							_pAnimator->SetBoolParam(*_pInstance, IS_GROUND_HASH, "IsGround", _isGround);
							_pAnimator->SetBoolParam(*_pInstance, IS_BOOST_HASH, "IsBoost", _isBoost);
							if (_pActionIntent)
							{
								_pAnimator->SetBoolParam(*_pInstance, IS_SHOOT_HASH, "IsShoot", _pActionIntent->IsAnyWeaponShoot());
							}
						};

					// 基本レイヤー
					_WriteParams(_animComp.baseLayer);

					// 上に重ねるレイヤー
					if (a_ctx.pWorld->HasComponent<Component::UpperAnimatorComponent>(_self))
					{
						if (const auto* _pUpper = a_ctx.pWorld->RefData<Component::UpperAnimatorComponent>(_self))
						{
							_WriteParams(_pUpper->layer);
						}
					}
				}
			}
		)
		// 絞り込みに使わない読み : 持っているときだけ RefData で読む。
		// 宣言しないと ActionIntent を書くジョブ(EnemyShootIntentSystem)と同時に走りうる
		.Reads<Component::ActionIntentComponent, Component::GroundStateComponent, Component::UpperAnimatorComponent, Component::BoostStateComponent, Component::ChargeDashComponent>();
	}
}
