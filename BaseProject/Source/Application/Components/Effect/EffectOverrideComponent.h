#pragma once

#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"	// EFFECT_PARAM_MAX

namespace App::Component
{
	//==========================================================================================
	// EffectOverrideComponent
	//
	// 出す側からの置き方の上書き。制御側のシステム(BoosterEffectSystem など)が毎フレーム書き、
	// EffectDrawSystem が読む。
	//
	// アセットは GUID 単位で全員に共有されるので、「同じ噴射でも取り付け位置と向きが
	// 個体ごとに違う」といった差はアセットへは書けない。かといって個体ごとに
	// アセットを増やすと、絵を1つ直すのに全部開いて回ることになる。
	// そこで「アセットが決めるのは中身、個体が決めるのは置き方」に分け、置き方はここへ書く。
	//
	// ・以前は EffectAssetComponent に入っていた。
	// ・EffectAssetComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
	//==========================================================================================
	struct EffectOverrideComponent
	{
		//------------------------------------------------------------------
		// 個体ごとのパラメータ(0〜1 を想定)
		//
		// 制御側のシステムが毎フレーム書く(ブーストの溜まり具合・速さなど)。
		// どこにどれだけ効くかはアセット側の結び付け(EffectAsset::GetParamBindings)が決めるので、
		// 書く側はアセットの中身を知らなくてよい。結び付けが無ければ何も変わらない
		//------------------------------------------------------------------
		float params[Engine::Resource::EFFECT_PARAM_MAX] = {};

		// エフェクト全体のスケール倍率。1 で等倍。
		// パーティクルの粒の大きさ・ばらつき半径・パーツの配置とメッシュにまとめて掛かる
		float effectScale = 1.0f;

		// 噴射の「長さ」の倍率。1 で等倍。
		// 粒の初速に掛かるので、寿命が同じなら飛ぶ距離＝束の長さがそのまま倍率ぶん伸びる。
		// effectScale と分けてあるのは、太さは変えずに長さだけ伸ばしたい場面
		// (チャージダッシュの撃ち出しなど)があるため。
		// あちらを上げると粒もばらつき半径も一緒に太るので、噴射が丸く膨らんでしまう
		float effectLengthScale = 1.0f;

		// 下の2つ(エフェクトの置き場)を使うか。false なら持ち主の行列にそのまま置く
		bool isOverrideTransform = false;

		//------------------------------------------------------------------
		// エフェクトの置き場 : オーナーの行列基準の位置と向き
		//
		// エフェクト全体をこの位置へ動かし、+Z がこの向きになるよう回してから置く。
		// パーツの位置と向きは置き場から見た相対なので、パーツの向きが +Z ならこの向きにそのまま一致する。
		// (以前は向きでパーツの向きを丸ごと差し替えていて、向きの違うパーツが全部同じ向きになっていた)
		//------------------------------------------------------------------
		Math::Vector3 overridePosOffset = { 0.0f, 0.0f, 0.0f };	// 置き場の位置
		Math::Vector3 overrideEmitDir = { 0.0f, 0.0f, 1.0f };	// 置き場の +Z の向き
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::EffectOverrideComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::EffectOverrideComponent& _comp = Engine::EditorField::GetValue<App::Component::EffectOverrideComponent>(a_context.pData);

		// 制御側のシステムが毎フレーム書くので表示だけ
		Engine::EditorField::Value("Scale", "%.2f", _comp.effectScale);
		Engine::EditorField::Value("LengthScale", "%.2f", _comp.effectLengthScale);
		if (_comp.isOverrideTransform)
		{
			Engine::EditorField::Value("Override Pos", "%.2f, %.2f, %.2f", _comp.overridePosOffset.x, _comp.overridePosOffset.y, _comp.overridePosOffset.z);
			Engine::EditorField::Value("Override Dir", "%.2f, %.2f, %.2f", _comp.overrideEmitDir.x, _comp.overrideEmitDir.y, _comp.overrideEmitDir.z);
		}
		else
		{
			Engine::EditorField::HelpText("(置き方はアセットのパーツ側)");
		}

		// 個体ごとのパラメータ(制御側が書く。効き先はアセットの結び付け)
		for (size_t _i = 0; _i < Engine::Resource::EFFECT_PARAM_MAX; ++_i)
		{
			const std::string _label = "Param " + std::to_string(_i);
			Engine::EditorField::Value(_label.c_str(), "%.2f", _comp.params[_i]);
		}
	}
};
