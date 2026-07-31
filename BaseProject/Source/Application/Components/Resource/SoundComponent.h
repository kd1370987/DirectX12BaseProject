#pragma once

#include "../../../Engine/Editor/EditorUI/EditorUI.h"
#include "../../../Engine/ECS/World/World.h"

struct SoundComponent
{
	Engine::GUID soundGUID = Engine::DefaultGUID;									// サウンド本体のGUID
	Engine::Handle<Engine::Resource::SoundInstance> soundInstanceHandle = {};		// サウンドから作られたインスタンスのハンドル
	float vol = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<SoundComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SoundComponent& _comp = Engine::Editor::GetValue<SoundComponent>(a_pData);
		a_ar.Field("SoundGUID",_comp.soundGUID);
		a_ar.Field("Vol",_comp.vol);
	}

	static void Edit(CompEditContext& a_context)
	{
		SoundComponent& _comp = Engine::Editor::GetValue<SoundComponent>(a_context.pData);
		if (Engine::Editor::UI::DrawAssetSelectComboGUID(
			"Change Sound",
			"Sound",
			_comp.soundGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる。
			// プレハブ編集では実体が無く entity は INVALID なので、
			// GUID の書き換えだけ行い、リフレッシュはしない(無効IDで参照するとレンジ外になる)。
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->AddRefreshEntity(a_context.entity);
			}
		}
	}
};