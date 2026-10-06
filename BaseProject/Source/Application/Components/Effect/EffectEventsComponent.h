#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorField.h"
#include "Engine/Editor/Helper/EditorField.inl"

//==========================================================================================
// EffectEventsComponent
//
// 「出来事が起きたら、その場にエフェクトを出す」の対応表。一発もの専用。
//
//   OnSpawn : 生まれたとき(Active になった最初のフレーム)に、自分の位置へ
//   OnDeath : 死んだとき(DeathEventResource)に、死んだ位置へ
//   OnHit   : 攻撃を受けたとき(HitEventResource の victim)に、当たった位置へ
//
// ・出すのは EffectAsset だけ。出したエンティティは destroyOnFinish で自分から消える(SpawnEffectAt)。
//   音もエフェクトのサウンドパーツで鳴らすので、ここで音を別に持たない。
// ・1エンティティに同じ型のコンポーネントは1つしか付けられないので、表にして複数持たせる。
//   (以前、被弾音を SoundComponent と別の HitSoundComponent にしていたのは、この制約のため)
// ・付いて出し続けるもの(ブースターの噴射など)は、今まで通り EffectAssetComponent を
//   子エンティティに付ける。こちらは一発ものだけを受け持つ。
// ・以前の DeathEffectComponent(OnDeath) / HitSoundComponent(OnHit) /
//   SoundComponent の isPlayOnSpawn(OnSpawn)を置き換えたもの(Phase 6 で移行して削除)。
// ・解決と先読み(Warmup)は EffectEventSystem の Fixup、出すのも EffectEventSystem。
//==========================================================================================

// 出来事の種類 ※ 値は保存されるので、増やすときは必ず末尾に足すこと
enum class EEffectEvent : uint32_t
{
	OnSpawn,	// 生まれたとき
	OnDeath,	// 死んだとき
	OnHit,		// 攻撃を受けたとき
};

// 表の大きさ。増やすときはここだけ変えればよい(保存は件数ぶんのキーで書く)
inline constexpr size_t EFFECT_EVENT_MAX = 4;

// 表の1行
struct EffectEventEntry
{
	EEffectEvent event = EEffectEvent::OnSpawn;
	Engine::GUID effectGUID = Engine::DEFAULT_GUID;							// 出すエフェクト(未設定なら何もしない)
	Engine::Handle<Engine::Resource::EffectAsset> effectHandle = {};		// ランタイム用(Fixup が解決する)
	float scale = 1.0f;														// エフェクト全体の大きさ倍率

	bool IsValid() const { return effectGUID != Engine::DEFAULT_GUID; }
};

struct EffectEventsComponent
{
	EffectEventEntry entries[EFFECT_EVENT_MAX] = {};

	// ---- ランタイム(保存しない) ----
	// OnSpawn をもう出したか。生まれて最初のフレームだけ出す(Fixup で下ろす)
	bool isSpawnFired = false;
};

template<>
struct Engine::ECS::ComponentTraits<EffectEventsComponent>
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
		EffectEventsComponent& _comp = Engine::Editor::GetValue<EffectEventsComponent>(a_pData);
		auto& _resourceManager = *a_services.pResourceManager;

		for (EffectEventEntry& _entry : _comp.entries)
		{
			_resourceManager.ReleaseHandle(_entry.effectHandle);
		}
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		EffectEventsComponent& _comp = Engine::Editor::GetValue<EffectEventsComponent>(a_pData);

		// ハンドルと出したかの印はランタイム状態なので保存しない
		for (size_t _i = 0; _i < EFFECT_EVENT_MAX; ++_i)
		{
			const std::string _key = "entries[" + std::to_string(_i) + "]";
			a_ar.Field(_key + ".event", _comp.entries[_i].event);
			a_ar.Field(_key + ".effectGUID", _comp.entries[_i].effectGUID);
			a_ar.Field(_key + ".scale", _comp.entries[_i].scale);
		}
	}

	static void Edit(CompEditContext& a_context)
	{
		EffectEventsComponent& _comp = Engine::Editor::GetValue<EffectEventsComponent>(a_context.pData);

		Engine::Editor::HelpText("出来事が起きたら、その場にエフェクトを出す(一発もの)");

		for (size_t _i = 0; _i < EFFECT_EVENT_MAX; ++_i)
		{
			EffectEventEntry& _entry = _comp.entries[_i];

			Engine::Editor::IDScope _id(static_cast<int>(_i));
			Engine::Editor::Header(("Entry " + std::to_string(_i)).c_str());

			Engine::Editor::Field("Event", _entry.event);
			switch (_entry.event)
			{
			case EEffectEvent::OnSpawn:	Engine::Editor::Tooltip("生まれたときに、自分の位置へ出す"); break;
			case EEffectEvent::OnDeath:	Engine::Editor::Tooltip("死んだときに、死んだ位置へ出す"); break;
			case EEffectEvent::OnHit:	Engine::Editor::Tooltip("攻撃を受けたときに、当たった位置へ出す"); break;
			default: break;
			}

			Engine::Editor::AssetField<Engine::Resource::EffectAsset>(
				*a_context.pWorld->RefEngineServices(),
				"Effect",
				"EffectAsset",
				_entry.effectGUID,
				_entry.effectHandle);

			Engine::Editor::Field("Scale", _entry.scale, 0.05f, 0.0f);

			if (!_entry.IsValid())
			{
				Engine::Editor::HelpText("(未設定 : 何も出ない)");
			}
		}
	}
};
