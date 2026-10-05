#include "SoundFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Audio/AudioBehaviorComponent.h"
#include "Engine/Audio/AudioManager.h"

//==========================================================================================
// SoundFixupSystem
//
// GUID しか保存されていないサウンドから、再生用インスタンスを発行し直す。
// サウンドを持つコンポーネントごとにタスクを登録する
// (同じ寿命の管理なので、システムを分けずここにまとめている)。
//
// AudioBehaviorComponent はファイル1つではなく「音の流れ」を指すので、
// まずアセットを解決してから、その定義に沿ってフェーズぶんのインスタンスを発行する。
//==========================================================================================
void SoundFixupSystem::Init(App::ECS::APPWorld& a_world)
{
	// オーディオビヘイビア。
	// アセット側に音量と3D指定まで入っているので、ここでは発行するだけでよい
	a_world.PostDeserializeTask<AudioBehaviorComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"AudioBehaviorFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			AudioBehaviorComponent* a_behaviorArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pAudioManager || !_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AudioBehaviorComponent& _behaviorComp = a_behaviorArray[_i];

				// 差し替え時のリフレッシュでは Release 済みだが、
				// それ以外の経路で残っていた場合の二重発行を防ぐ
				_behaviorComp.instance.Release(*_pAudioManager);

				if (_behaviorComp.behaviorGUID == Engine::DefaultGUID)
				{
					_behaviorComp.behaviorHandle = {};
					continue;
				}

				// ビヘイビアアセットを解決
				_pResourceManager->AcquireImmediate(_behaviorComp.behaviorHandle, _behaviorComp.behaviorGUID);

				auto* _pBehavior = _pResourceManager->Ref(_behaviorComp.behaviorHandle);
				if (!_pBehavior) continue;

				// 定義に沿ってフェーズぶんの再生用インスタンスを発行する。
				// 音が入っていないフェーズは無効ハンドルのまま残る
				_pBehavior->CreateInstance(*_pAudioManager, _behaviorComp.instance);
			}
		}
	);
}
