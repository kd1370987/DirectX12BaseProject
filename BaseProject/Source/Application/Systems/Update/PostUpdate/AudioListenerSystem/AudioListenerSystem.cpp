#include "AudioListenerSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Audio/AudioManager.h"

#include "Application/Components/Resource/AudioListenerComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

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
// ・普通の ActiveTask ではなくカスタムタスクで登録している。
//   ActiveTask は ActiveTag を「書く」扱いになり、一方 ActiveCustomTask である
//   CommitHierarchyWorldMatrixSystem は ActiveTag を「読む」うえに WorldMatrix を書く。
//   そのため PostUpdate 帯で WorldMatrix を読む ActiveTask を作ると、
//   互いを指してシステムのソートが循環する。読み書きを明示できるカスタムタスクなら
//   ActiveTag を書かずに済むので、行列確定の後ろに素直に並ぶ。
//   (同じ理由で FlyingSoundSystem もカスタムタスクにしている)
//==============================================================================
void AudioListenerSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<WorldMatrixComponent>{},
		Engine::ECS::WriteList<AudioListenerComponent>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;

			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			const float _dt = a_ctx.dt;

			a_ctx.pWorld->ForEach<ActiveTag, AudioListenerComponent, WorldMatrixComponent>(
				[_pAudioManager, _dt](
					Engine::ECS::ArchetypeChunk* a_pChunk,
					uint32_t                     a_count,
					ActiveTag*                   a_tags,
					AudioListenerComponent*      a_listenerArray,
					WorldMatrixComponent*        a_worldMatArray)
				{
					for (uint32_t _i = 0; _i < a_count; ++_i)
					{
						AudioListenerComponent&     _listener  = a_listenerArray[_i];
						const WorldMatrixComponent& _worldComp = a_worldMatArray[_i];

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
