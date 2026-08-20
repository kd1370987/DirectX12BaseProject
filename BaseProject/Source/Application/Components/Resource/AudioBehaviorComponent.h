#pragma once

#include "../../../Engine/Editor/Helper/EditorHelper.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Audio/AudioManager.h"
#include "../../../Engine/Resource/Data/AudioBehavior/AudioBehavior.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

//==========================================================================================
// AudioBehaviorComponent
//
// 始動時・継続中・終了時の音をひとまとめにした AudioBehavior アセットを付ける。
//
// SoundComponent が「1つのファイルを鳴らす」のに対して、こちらは「音の流れ」を持つ。
// 鳴らす側のシステムは Start / Loop / End を呼ぶだけでよく、
// どのフェーズに何の音が入っているか(あるいは入っていないか)を知らなくてよい。
//
// アセットは全員で共有する設計図なので、再生用インスタンスはこちらの instance が持つ
// (発行 : AudioBehaviorFixupSystem / 返却 : AudioBehaviorFreeSystem)。
//==========================================================================================
struct AudioBehaviorComponent
{
	// ビヘイビアアセットのGUID(保存されるのはこれだけ)
	Engine::GUID behaviorGUID = Engine::DefaultGUID;

	// GUIDから解決したアセットのハンドル
	Engine::Handle<Engine::Resource::AudioBehavior> behaviorHandle = {};

	// このエンティティ専用の再生用インスタンス
	Engine::Resource::AudioBehaviorInstance instance = {};
};

template<>
struct Engine::ECS::ComponentTraits<AudioBehaviorComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AudioBehaviorComponent& _comp = Engine::Editor::GetValue<AudioBehaviorComponent>(a_pData);
		a_ar.Field("BehaviorGUID", _comp.behaviorGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		AudioBehaviorComponent& _comp = Engine::Editor::GetValue<AudioBehaviorComponent>(a_context.pData);

		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Change AudioBehavior",
			"AudioBehavior",
			_comp.behaviorGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる。
			// プレハブ編集では実体が無く entity は INVALID なので、
			// GUID の書き換えだけ行い、リフレッシュはしない(無効IDで参照するとレンジ外になる)。
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->AddRefreshEntity(a_context.entity);
			}
		}

		if (_comp.behaviorGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : 何も鳴らない)");
			return;
		}

		// 中身の確認用。細かい編集はアセット側のインスペクターで行う
		auto* _pBehavior = Engine::Resource::ResourceManager::Instance().Ref(_comp.behaviorHandle);
		if (!_pBehavior)
		{
			ImGui::TextDisabled("(読み込み中)");
			return;
		}

		ImGui::Separator();
		for (size_t _i = 0; _i < Engine::Resource::AUDIO_PHASE_COUNT; ++_i)
		{
			const auto _phase = static_cast<Engine::Resource::EAudioPhase>(_i);
			const bool _hasPart = _pBehavior->HasPart(_phase);

			ImGui::Text("%s : %s",
				Engine::Resource::ToString(_phase),
				_hasPart ? "assigned" : "empty");
		}
	}
};
