#pragma once

#include "AnimatorComponent.h"

//==========================================================================================
// UpperAnimatorComponent
//
// 基本レイヤー(AnimatorComponent::baseLayer)の上に重ねるアニメーター。
//
// ・基本レイヤーがノードポーズの配列へ書いた後に、ボーンレイヤー(モデルの BoneMask)に
//   載っているノードだけを、このレイヤーのクリップで上書きする(AnimationSystem の UpperAnimationSystem)。
//   マスクの重みが 0〜1 の間なら、基本レイヤーの結果と TRS で補間する。
// ・ボーンレイヤーが未選択(0)なら全身を上書きする。
// ・ステートの遷移・パラメータの実体は基本レイヤーとは別に持つ(設計図も別)。
//==========================================================================================
struct UpperAnimatorComponent
{
	AnimatorLayer layer = {};

	// レイヤー全体の効き(0〜1)。マスクの重みに掛ける。外から書けばフェードに使える
	float weight = 1.0f;
};

template<>
struct Engine::ECS::ComponentTraits<UpperAnimatorComponent>
{
	// 重ねる土台が要る(ポーズの置き場なども AnimatorComponent から推移的に付く)
	using Requires = Engine::ECS::RequireComponents<AnimatorComponent>;

	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	// (パラメータの実体はワールドのプールにあるので、ここではなく AnimatorFreeSystem が返す)
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		UpperAnimatorComponent& _comp = Engine::Editor::GetValue<UpperAnimatorComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.layer.animatorHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		UpperAnimatorComponent& _comp = Engine::Editor::GetValue<UpperAnimatorComponent>(a_pData);

		if (a_ar.BeginGroup("layer"))
		{
			a_ar.Field("animatorGUID", _comp.layer.animatorGUID);
			a_ar.Field("boneMaskHash", _comp.layer.boneMaskHash);
			a_ar.EndGroup();
		}
		a_ar.Field("weight", _comp.weight);
	}

	static void Edit(CompEditContext& a_context)
	{
		UpperAnimatorComponent& _comp = Engine::Editor::GetValue<UpperAnimatorComponent>(a_context.pData);

		Engine::Editor::Slider("Weight", _comp.weight, 0.0f, 1.0f, "%.2f");
		EditAnimatorLayer(a_context, "Upper Layer", _comp.layer, true);
	}
};
