#include "AudioListenerSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Audio/AudioManager.h"

#include "Application/Components/Audio/AudioListenerComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

namespace App::System
{
	//==============================================================================
	// AudioListenerSystem
	//
	// 聞き手(プレイヤー)の位置・向き・速度を毎フレーム AudioManager へ送る。
	// 3D再生している音は Apply3D のたびに AudioManager のリスナーを見るので、
	// ここが送らないと全部「原点で +Z を向いている人」が聞いた音になる。
	//
	// ・姿勢はワールド行列から取る。左手系なので前方は +Z 軸(第3行)、上は +Y 軸(第2行)。
	//   SimpleMath の Vector3::Forward は -Z なので使わない。
	// ・速度は位置の差分から出す(ドップラー用)。テレポートで爆音にならないよう、
	//   1フレーム目や dt が 0 のときは 0 のままにする。
	// ・PostUpdate 帯に置く。ワールド行列が確定した後に読みたいため。
	// ・カスタムタスクで登録している。WorldMatrix を ReadList で読むので、
	//   行列確定(CommitHierarchyWorldMatrixSystem)の後ろに並ぶ。
	//   (以前は ActiveTask が ActiveTag を書く扱いになり、ソートが循環するのを避けるためだった。
	//    今はフェーズのタグを依存に数えない(IsQueryOnlyTag)ので、その心配は無い。
	//    以前あった FlyingSoundSystem も同じ経緯でカスタムタスクになっていた)
	//==============================================================================
	void AudioListenerSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveCustomTask(
			Engine::ECS::ESystemType::PostUpdate,
			"AudioListenerSystem",
			Engine::ECS::ReadList<Component::WorldMatrixComponent>{},
			Engine::ECS::WriteList<Component::AudioListenerComponent>{},
			[](const Engine::ECS::SystemContext& a_ctx)
			{
				if (!a_ctx.pWorld) return;

				auto* _pAudioManager = a_ctx.pServices->pAudioManager;
				if (!_pAudioManager) return;

				const float _dt = a_ctx.dt;

				a_ctx.pWorld->ForEach<Component::ActiveTag, Component::AudioListenerComponent, Component::WorldMatrixComponent>(
					[_pAudioManager, _dt](
						Engine::ECS::Chunk* a_pChunk,
						uint32_t                     a_count,
						Component::ActiveTag*                   a_tags,
						Component::AudioListenerComponent*      a_listenerArray,
						Component::WorldMatrixComponent*        a_worldMatArray)
					{
						for (uint32_t _i = 0; _i < a_count; ++_i)
						{
							Component::AudioListenerComponent&     _listener  = a_listenerArray[_i];
							const Component::WorldMatrixComponent& _worldComp = a_worldMatArray[_i];

							Math::Matrix _world(_worldComp.worldMat);

							Engine::Audio::ListenerData _data = {};

							// 耳の位置(ローカルオフセットをワールドへ)
							_data.pos = Math::Vector3::Transform(Math::Vector3(_listener.posOffset), _world);

							// 左手系 : +Z が前方、+Y が上
							_data.front = Math::Vector3(_world._31, _world._32, _world._33);
							_data.up    = Math::Vector3(_world._21, _world._22, _world._23);

							// 速度 : 前フレームからの移動量
							if (_listener.useVelocity && _listener.hasPrevPos && _dt > 0.0f)
							{
								_data.velocity = (_data.pos - Math::Vector3(_listener.prevPos)) / _dt;
							}

							_listener.prevPos    = _data.pos;
							_listener.hasPrevPos = true;

							_pAudioManager->SubmitListener(_data);
						}
					}
				);
			}
		);
	}
}
