#pragma once

#include "../../../Engine/Editor/Helper/EditorHelper.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Audio/AudioManager.h"

//==========================================================================================
// HitSoundComponent
//
// 「攻撃を受けたときに、自分から鳴らす音」を持つコンポーネント。
// 付けたエンティティが HitEventResource の victim になったフレームに、
// HitSoundSystem がこの音を鳴らす。
//
// SoundComponent とは別に用意している。1エンティティが持てる SoundComponent は
// 1つで、プレイヤーのそれは既にブーストの継続音(ループ)が使っているため、
// 同じものを被弾音に流用すると踏み合いになる。
//
// サウンドインスタンスの発行・返却は SoundComponent と同じ流れに乗せている
// (発行 : SoundFixupSystem / 返却 : SoundFreeSystem)。
//==========================================================================================
struct HitSoundComponent
{
	Engine::GUID soundGUID = Engine::DefaultGUID;									// サウンド本体のGUID
	Engine::Handle<Engine::Resource::SoundInstance> soundInstanceHandle = {};		// サウンドから作られたインスタンス

	float vol = 1.0f;				// 音量
	float minInterval = 0.1f;		// 鳴らし直す最短間隔 : 秒。
									// インスタンスは1つしか無く Play が鳴り直しになるので、
									// 連射を受けている間に頭出しを繰り返して潰れるのを防ぐ。
									// 0 なら毎ヒット鳴らし直す

	float coolTime = 0.0f;			// ランタイム : 次に鳴らせるまでの残り時間(保存しない)
};

template<>
struct Engine::ECS::ComponentTraits<HitSoundComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		HitSoundComponent& _comp = Engine::Editor::GetValue<HitSoundComponent>(a_pData);
		a_ar.Field("SoundGUID", _comp.soundGUID);
		a_ar.Field("Vol", _comp.vol);
		a_ar.Field("MinInterval", _comp.minInterval);
	}

	static void Edit(CompEditContext& a_context)
	{
		HitSoundComponent& _comp = Engine::Editor::GetValue<HitSoundComponent>(a_context.pData);

		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Change Sound",
			"Sound",
			_comp.soundGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる
			// (プレハブ編集では実体が無く entity は無効値)
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->AddRefreshEntity(a_context.entity);
			}
		}

		// 音量は発行済みインスタンスへ即時反映して、鳴らしながら調整できるようにする
		if (ImGui::DragFloat("Volume", &_comp.vol, 0.01f, 0.0f, 1.0f))
		{
			auto* _pInstance = Engine::Audio::AudioManager::Instance().RefInstance(_comp.soundInstanceHandle);
			if (_pInstance) _pInstance->SetVolume(_comp.vol);
		}

		if (ImGui::DragFloat("MinInterval", &_comp.minInterval, 0.01f, 0.0f, 10.0f, "%.2f s"))
		{
			if (_comp.minInterval < 0.0f) _comp.minInterval = 0.0f;
		}
	}
};
