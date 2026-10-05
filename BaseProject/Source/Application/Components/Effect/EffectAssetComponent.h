#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorField.h"
#include "Engine/ECS/World/World.h"

#include "EffectRuntimeComponent.h"
#include "EffectPlayRequestComponent.h"
#include "EffectOverrideComponent.h"

//==========================================================================================
// EffectAssetComponent
//
// パーティクルとメッシュをまとめた EffectAsset を再生するコンポーネント(設定。保存される)。
//
// 「ジェット噴射」「爆発」といった演出ひとまとまり(パーティクル・メッシュ・音・ライト)を1枚のアセットで扱う。
// パーティクルや音を単体でエンティティに付けることはしない(どれもエフェクトのパーツ)。
// 制御側のシステムは再生の要求(EffectPlayRequestComponent)を書くだけでよく、
// 何個のパーティクルとメッシュで出来ているかを知らなくてよい。
//
// 実行中の値は次の3つに分けてあり、どれも必須コンポーネントとして自動で付く。
//   EffectRuntimeComponent     … 解決したハンドルと進行状態(EffectFixup / EffectUpdate / EffectDraw)
//   EffectPlayRequestComponent … 再生させたいか(制御側が書く)
//   EffectOverrideComponent    … 置き方の上書き(制御側が書く)
// 以前は1つの型に全部入っていて、要求を出すだけの側まで進行状態の書き手とぶつかっていた。
//==========================================================================================
struct EffectAssetComponent
{
	// エフェクトアセットのGUID
	Engine::GUID effectGUID = Engine::DefaultGUID;

	// 生成された時点から再生するか。
	// 爆発のように出しっぱなしで完結するものはこれを立てる。
	// 噴射のように状況で入り切りするものは false にして、制御側が再生の要求を書く
	bool playOnStart = false;

	// 全パーツを出し終わったら自分ごと消えるか。
	// 単発エフェクトのプレハブに立てておくと、後片付けが要らなくなる
	// (出しっぱなしのパーツが1つでもあると終わらないので、その場合は何も起きない)
	bool destroyOnFinish = false;
};

template<>
struct Engine::ECS::ComponentTraits<EffectAssetComponent>
{
	// 実行中の値は保存しないので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<EffectRuntimeComponent, EffectPlayRequestComponent, EffectOverrideComponent>;

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		EffectAssetComponent& _comp = Engine::Editor::GetValue<EffectAssetComponent>(a_pData);
		a_ar.Field("EffectGUID", _comp.effectGUID);
		a_ar.Field("PlayOnStart", _comp.playOnStart);
		a_ar.Field("DestroyOnFinish", _comp.destroyOnFinish);
	}

	static void Edit(CompEditContext& a_context)
	{
		EffectAssetComponent& _comp = Engine::Editor::GetValue<EffectAssetComponent>(a_context.pData);

		if (Engine::Editor::AssetField(
			*a_context.pWorld->RefEngineServices(),
			"Effect",
			"EffectAsset",
			_comp.effectGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる。
			// プレハブ編集では実体が無く entity は INVALID なので、
			// GUID の書き換えだけ行い、リフレッシュはしない(無効IDで参照するとレンジ外になる)
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->ReserveRefreshEntity(a_context.entity);
			}
		}

		// 出っぱなしにするか。切り替えは即座に反映して、エディタで確認できるようにする
		// (生成時の反映は EffectFixupSystem が行う)
		if (Engine::Editor::Field("PlayOnStart", _comp.playOnStart) &&
			a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
		{
			if (auto* _pRequest = a_context.pWorld->RefData<EffectPlayRequestComponent>(a_context.entity))
			{
				_pRequest->isPlay = _comp.playOnStart;
			}
		}
		Engine::Editor::Field("DestroyOnFinish", _comp.destroyOnFinish);
		Engine::Editor::Tooltip("出し切ったら自分ごと消す(出しっぱなしのパーツがあると消えない)");

		if (_comp.effectGUID == Engine::DefaultGUID)
		{
			Engine::Editor::HelpText("(未設定 : 何も出ない)");
		}
	}
};
