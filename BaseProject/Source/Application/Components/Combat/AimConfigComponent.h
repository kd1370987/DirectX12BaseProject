#pragma once

#include "AimResultComponent.h"

//==========================================================================================
// AimConfigComponent
//
// 狙点を求めるレイの設定。保存される。読むのは AimTargetSystem だけ。
//
// ・カメラがフォーカスしている対象(プレイヤーなど)と、狙点を受け取る銃に付ける。
//   結果(AimResultComponent)は必須コンポーネントとして自動で付く。
// ・以前の AimTargetPosComponent から結果を分けた残り。
//==========================================================================================
struct AimConfigComponent
{
	float maxDistance = 500.0f;		// レイの長さ。ここまで当たらなければ「最大距離の点」を狙点にする
	float startOffset = 1.0f;		// フォーカス点(自機)から何m先をレイの始点にするか。
									// 自機に付いている武器・ブースターを拾わないための余白。
									// カメラからの絶対距離ではないので、カメラ距離を変えても調整不要。
};

template<>
struct Engine::ECS::ComponentTraits<AimConfigComponent>
{
	// 結果は毎フレーム書き直す値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<AimResultComponent>;

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AimConfigComponent& _comp = Engine::Editor::GetValue<AimConfigComponent>(a_pData);
		a_ar.Field("maxDistance", _comp.maxDistance);
		a_ar.Field("startOffset", _comp.startOffset);
	}

	static void Edit(CompEditContext& a_context)
	{
		AimConfigComponent& _comp = Engine::Editor::GetValue<AimConfigComponent>(a_context.pData);
		Engine::Editor::Field("MaxDistance", _comp.maxDistance, 1.0f, 0.0f);
		Engine::Editor::Field("StartOffset", _comp.startOffset, 0.1f, 0.0f);
	}
};
