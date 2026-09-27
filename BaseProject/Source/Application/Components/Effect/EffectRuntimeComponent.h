#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// EffectRuntimeComponent
//
// EffectAssetComponent の実行中の値 : GUID から解決したアセットのハンドルと、
// このエンティティ専用の進行状態。
//
// ・解決は EffectFixupSystem、進行は EffectUpdateSystem、発生・描画は EffectDrawSystem。
// ・アセットは全員で共有する設計図なので、進行状態は instance が持つ。
// ・EffectAssetComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
//==========================================================================================
struct EffectRuntimeComponent
{
	// GUID から解決したアセットのハンドル
	Engine::Handle<Engine::Resource::EffectAsset> effectHandle = {};

	// このエンティティ専用の進行状態
	Engine::Resource::EffectInstance instance = {};
};

template<>
struct Engine::ECS::ComponentTraits<EffectRuntimeComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事。
	// ECS がエンティティを消すとき・コンポーネントを外すとき・
	// PostDeserialize へ入り直すとき(fixup が取り直す)に必ず呼ぶ。
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		EffectRuntimeComponent& _comp = Engine::Editor::GetValue<EffectRuntimeComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.effectHandle);
	}

	static void Edit(CompEditContext& a_context)
	{
		EffectRuntimeComponent& _comp = Engine::Editor::GetValue<EffectRuntimeComponent>(a_context.pData);

		// 中身の確認用。細かい編集はアセット側のインスペクターで行う
		auto* _pEffect = a_context.pWorld->RefEngineServices()->pResourceManager->Ref(_comp.effectHandle);
		if (!_pEffect)
		{
			Engine::Editor::HelpText("(未解決 / 読み込み中)");
			return;
		}

		Engine::Editor::Value("Particle Parts", "%d", static_cast<int>(_pEffect->GetParticleParts().size()));
		Engine::Editor::Value("Mesh Parts", "%d", static_cast<int>(_pEffect->GetMeshParts().size()));
		Engine::Editor::Value("Elapsed", "%.2f", _comp.instance.elapsed);
	}
};
