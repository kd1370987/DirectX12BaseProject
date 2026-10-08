#include "MainCameraSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Camera/CameraTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

namespace App::System
{
	//==========================================================================================
	// MainCameraSystem
	//
	// 映すカメラを1台だけ決めて SingletonEntityResource.mainCamera へ置く。
	//
	// ・どれが映すかはカメラ自身(CameraParamComponent.isActive)が持っている。
	//   もとは ActiveCameraTag という空のタグで、使う側それぞれが
	//   ForEach でタグ付きのカメラを探し直していた。
	//   探し方が使う側ごとに書かれていると、条件のわずかな違いで
	//   「絵は新しいカメラなのにロックオンは前のカメラで判定している」が起きる。
	//   ここで一度だけ決めて、使う側は答えを受け取るだけにする。
	//
	// ・PreUpdate に置く理由
	//     使う側は Camera(AimTargetSystem)・PostUpdate(LockOn / MissileSalvo)・
	//     PreDraw(CamSetShaderSystem)と後ろの帯に散っている。
	//     リソースの読み書きはシステムのソートに出ない(依存の辺にならない)ので、
	//     順番は帯そのもので作るしかない。いちばん手前の帯へ置いておけば、
	//     どの使い手も「今フレームの答え」を読める。
	//
	// ・立っているカメラが複数あるときは先に見つかったものが勝つ。
	//   カメラを切り替えるときは前のカメラの isActive を必ず下ろすこと。
	//
	// ・普通の ActiveTask ではなくカスタムタスクで登録している。
	//   ActiveTask はチャンクごとに呼ばれるので、「最初の1台」を選ぶには
	//   呼び出しをまたいだ状態が要るため。
	//==========================================================================================
	void MainCameraSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveCustomTask(
			Engine::ECS::ESystemType::PreUpdate,
			"MainCameraSystem",
			Engine::ECS::ReadList<Component::CameraTag, Component::CameraParamComponent>{},
			Engine::ECS::WriteList<>{},
			[](const Engine::ECS::SystemContext& a_ctx)
			{
				if (!a_ctx.pWorld) return;
				if (!a_ctx.pWorld->HasResource<InstanceResource::SingletonEntityResource>()) return;

				auto& _singleton = a_ctx.pWorld->RefResource<InstanceResource::SingletonEntityResource>();

				// 前のフレームの答えは捨てる。
				// カメラが消えた/全部下ろされたフレームは
				// 「メインカメラは居ない」が正しい状態になる
				_singleton.mainCamera = Engine::ECS::Limits::INVALID_ENTITY;

				a_ctx.pWorld->ForEach<const Component::ActiveTag, const Component::CameraTag, const Component::CameraParamComponent>(
					[&_singleton](
						Engine::ECS::Chunk*   a_pChunk,
						uint32_t                       a_count,
						const Component::ActiveTag*               /*a_tags*/,
						const Component::CameraTag*               /*a_camTagArray*/,
						const Component::CameraParamComponent*    a_camParamArray
					)
					{
						// もう決まっているなら残りのチャンクは見ない
						if (_singleton.HasMainCamera()) return;

						for (uint32_t _i = 0; _i < a_count; ++_i)
						{
							if (!a_camParamArray[_i].isActive) continue;

							_singleton.mainCamera = a_pChunk->entityData[_i];
							return;
						}
					}
				);
			}
		)
		// 答え(メインカメラ)の置き場
		.WritesResource<InstanceResource::SingletonEntityResource>();
	}
}
