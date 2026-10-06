#include "GunStateStartSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Weapon/GunStateComponent.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"

namespace App::System
{
	void GunStateStartSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.StartTask<const Component::ModelComponent,Component::GunStateComponent>(
			Engine::ECS::ESystemType::Start,
			"GunStateStartSystem",
			[](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::StartTag* a_tag,
				const Component::ModelComponent* a_modelArray,
				Component::GunStateComponent* a_gunStateArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::ModelComponent& _modelComp = a_modelArray[_i];
					Component::GunStateComponent& _gunComp = a_gunStateArray[_i];
					const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

					//==============================================================
					// マズルフラッシュの再生枠を銃自身へ持たせる
					//--------------------------------------------------------------
					// 撃つたびにエフェクト用のエンティティを生やすと、そのエンティティは
					// エフェクトが終わるまで残る。ワールドに置きっぱなしなので、
					// 移動しながら撃つと撃った位置に取り残されて尾を引いて見える。
					//
					// 銃自身に枠を1つ持たせ、撃つたびに頭から再生し直す形にする。
					// 銃に付いて動くようになり、エンティティも増えない。
					// 連射で前の1発がまだ消えていなくても上書きで出し直す
					// (MuzzleFlashSystem が EffectAsset::Play を直接呼ぶ)。
					//
					// 枠は EffectAssetComponent。進行(EffectUpdateSystem)も
					// 発生・描画(EffectDrawSystem)もこれを見ているので、付けるだけで動く。
					//==============================================================
					if (_gunComp.muzzleEffectGUID != Core::DEFAULT_GUID)
					{
						if (a_ctx.pWorld->HasComponent<Component::EffectAssetComponent>(_self))
						{
							// すでに別の用途で付いている銃からは奪わない。
							// 奪うと元の演出が止まるので、気付けるように知らせる
							ENGINE_WARNING(
								"[Gun] EffectAssetComponent を既に持っているため"
								"マズルフラッシュを設定できません");
						}
						else
						{
							// 渡すのは設定だけ。付けると初期化から入り直すので、
							// ハンドル(自分のぶんの参照)は EffectFixupSystem が GUID から取る。
							// 大きさは撃つたびに MuzzleFlashSystem が入れる
							Component::EffectAssetComponent _effectComp = {};
							_effectComp.effectGUID = _gunComp.muzzleEffectGUID;
							_effectComp.playOnStart = false;		// 撃つまでは出さない
							_effectComp.destroyOnFinish = false;	// 枠は銃と一緒に消えるまで残す

							const auto _typeID = a_ctx.pWorld->GetCompTypeID<Component::EffectAssetComponent>();
							if (_typeID != Engine::ECS::Limits::INVALID_COMPONENTTYPEID)
							{
								a_ctx.pWorld->ReserveAddComponent(
									_typeID, _self, reinterpret_cast<uint8_t*>(&_effectComp));
							}
						}
					}

					// 親のモデルを取得
					const auto* _pParentModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
					if (!_pParentModel) continue;

					// モデルのノードを検索し、ハッシュ一致するノードのインデックスを解決
					for (UINT _nodeIdx = 0; _nodeIdx < _pParentModel->GetOriginalNodeVec().size(); ++_nodeIdx)
					{
						const auto& _node = _pParentModel->GetOriginalNodeVec()[_nodeIdx];

						// 違うのならスキップ
						if (_node.nodeNameHash != _gunComp.nullPtrNodeHash) continue;
						_gunComp.nodeIndex = _nodeIdx;
					}
				}
			}
		);
	}
}
