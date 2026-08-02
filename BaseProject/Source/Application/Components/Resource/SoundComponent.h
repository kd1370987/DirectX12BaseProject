#pragma once

#include "../../../Engine/Editor/Helper/EditorHelper.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Audio/AudioManager.h"

struct SoundComponent
{
	Engine::GUID soundGUID = Engine::DefaultGUID;									// サウンド本体のGUID
	Engine::Handle<Engine::Resource::SoundInstance> soundInstanceHandle = {};		// サウンドから作られたインスタンスのハンドル
	float vol = 1.0f;
	bool isLoop = false;															// ループ再生するか : 鳴らす側がこの設定を見て Play する
};

template<>
struct Engine::ECS::ComponentTraits<SoundComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SoundComponent& _comp = Engine::Editor::GetValue<SoundComponent>(a_pData);
		a_ar.Field("SoundGUID",_comp.soundGUID);
		a_ar.Field("Vol",_comp.vol);
		a_ar.Field("IsLoop",_comp.isLoop);
	}

	static void Edit(CompEditContext& a_context)
	{
		SoundComponent& _comp = Engine::Editor::GetValue<SoundComponent>(a_context.pData);
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
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

		// ループ再生するかどうか。
		// 鳴らす側のシステムが Play(isLoop) で参照する。
		// (例: ブースト継続音は true、発進音のような単発は false)
		ImGui::Checkbox("Loop", &_comp.isLoop);

		// 音量は発行済みインスタンスへ即時反映して、鳴らしながら調整できるようにする
		if (ImGui::DragFloat("Volume", &_comp.vol, 0.01f, 0.0f, 1.0f))
		{
			auto* _pInstance = Engine::Audio::AudioManager::Instance().RefInstance(_comp.soundInstanceHandle);
			if (_pInstance) _pInstance->SetVolume(_comp.vol);
		}
	}
};