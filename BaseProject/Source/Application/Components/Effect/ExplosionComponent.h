#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// ExplosionComponent
//
// 「時間差で複数のエフェクトを順番に炊く」ための進行役。
// 生まれた瞬間から経過時間を数え、パーツごとの emitTime に達したら
// そのプレハブを自分と同じ場所に出す。全部出し終わったら自分は消える。
//
// ・発生のタイミングだけを握る。各パーツの動きや使うパーティクルアセットは
//   出される側のプレハブが持つので、こちらは何を出すかを知らなくてよい。
// ・出すのも数えるのも ExplosionSystem。
// ・出したプレハブは別エンティティなので、こちらが消えても道連れにはならない。
//   出したものの後片付けは、そのプレハブの LifeTimeComponent に任せる。
// ・DeathEffectComponent から出されるプレハブに付けておくと、
//   「死亡 → 爆発の演出一式」が丸ごとエフェクト側で完結する。
//==========================================================================================

/// <summary>
/// 1パーツぶんの「いつ・何を出すか」
/// </summary>
struct PartsEffect
{
	// エフェクトエンティティと発生タイミング
	Engine::GUID prefabGUID = {};								// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> prefabHandle = {};	// ランタイム用(初回に解決)
	float emitTime = 0.0f;										// 生成からの経過時間が これ を超えたら出す(秒)
	bool  isEmitted = false;									// 出したか(ランタイム。プレハブから実体化されるたび false に戻る)
};

/// <summary>
/// 発生のタイミングのみ握っておく。各自の動きや使うパーティクルアセットなどは各プレハブが管理する。
/// すべてのプレハブが動作したら、自身を消去する
/// </summary>
struct ExplosionComponent
{
	static constexpr int PARTS_MAX = 3;	// 同時に持てるパーツ数。増やすときはここだけ変えればよい

	float elapsedTime = 0.0f;			// 生成からの経過時間(秒)

	PartsEffect parts[PARTS_MAX] = {};	// 空きスロット(プレハブ未設定)は出すものが無いので済んだ扱いになる
};

template<>
struct Engine::ECS::ComponentTraits<ExplosionComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ExplosionComponent& _comp = Engine::Editor::GetValue<ExplosionComponent>(a_pData);

		// 経過時間と発生済みフラグはランタイム状態なので保存しない
		for (int _i = 0; _i < ExplosionComponent::PARTS_MAX; ++_i)
		{
			const std::string _key = "parts[" + std::to_string(_i) + "]";
			a_ar.Field(_key + ".prefabGUID", _comp.parts[_i].prefabGUID);
			a_ar.Field(_key + ".emitTime", _comp.parts[_i].emitTime);
		}
	}

	static void Edit(CompEditContext& a_context)
	{
		ExplosionComponent& _comp = Engine::Editor::GetValue<ExplosionComponent>(a_context.pData);

		for (int _i = 0; _i < ExplosionComponent::PARTS_MAX; ++_i)
		{
			PartsEffect& _parts = _comp.parts[_i];

			ImGui::PushID(_i);
			ImGui::SeparatorText(("Parts " + std::to_string(_i)).c_str());

			if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Effect Prefab", "Prefab", _parts.prefabGUID))
			{
				_parts.prefabHandle = {};	// GUIDが変わったら作り直し
			}
			ImGui::DragFloat("EmitTime", &_parts.emitTime, 0.05f, 0.0f);

			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::Text("ElapsedTime : %.2f", _comp.elapsedTime);
		ImGui::TextDisabled("全パーツを出し終えたら自分は消える");
	}
};
