#pragma once

#include "../../../../Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// BoosterEffectComponent
//
// ブースター(スラスター)の噴射エフェクト専用の設定。
// 噴射のエンティティに EffectAssetComponent と一緒に付ける。
//
// ・持つのは「置き方」と「吹かした瞬間の膨らみ」だけ。
//   何を出すか(粒の絵・寿命・色)は EffectAsset の持ち物で、こちらは触らない。
//
// ・置き方をここが持つ理由
//     エフェクトアセットは GUID 単位で全員に共有される。ブースターは機体の
//     左右・腕・脚で取り付け位置も向きも違うので、そこをアセットに書くと
//     ブースターの数だけアセットを増やすことになり、絵を1つ直すのに
//     全部を開いて回る羽目になる。
//     「アセット = 中身 / コンポーネント = 置き方」で分けてある。
//     実際の反映は BoosterEffectSystem が EffectAssetComponent の
//     上書き欄(overridePosOffset / overrideEmitDir / effectScale)へ書き込む。
//
// ・吹かした瞬間の膨らみ
//     点火(isPlay の立ち上がり)で burstScale まで一気に太らせ、
//     burstTime かけて baseScale へ戻す。
//     噴射が出っぱなしの一定量だと、吹かし始めの「ドンッ」という手応えが出ない。
//
// ・以前はパーティクルコンポーネント(ParticlesComponent)を直に付けていたが、
//   あちらは汎用なので、ブースター1つを調整するのに
//   発生量・寿命・絵といった「アセットに書くべきもの」まで並んでしまっていた。
//==========================================================================================
struct BoosterEffectComponent
{
	// ---- 取り付け(設定値) ----
	// どちらもこのエンティティの行列を基準にしたローカル値
	Math::Vector3 posOffset = { 0.0f, 0.0f, 0.0f };		// 噴射口の位置
	Math::Vector3 emitDir = { 0.0f, 0.0f, -1.0f };		// 噴射する向き(正規化はシステム側で行う)

	// ---- 吹かした瞬間の膨らみ(設定値) ----
	float baseScale = 1.0f;		// 通常時のスケール倍率
	float burstScale = 1.8f;	// 点火した瞬間のスケール倍率(baseScale より小さくすると縮んでから戻る)
	float burstTime = 0.18f;	// baseScale へ戻るまでの秒数。0 なら膨らませない

	// ---- ランタイム(保存しない) ----
	float burstTimer = 0.0f;	// 戻るまでの残り時間
	bool  wasPlaying = false;	// 点火の立ち上がりを見るための前フレームの状態
};

template<>
struct Engine::ECS::ComponentTraits<BoosterEffectComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_pData);

		a_ar.Field("posOffset", _comp.posOffset);
		a_ar.Field("emitDir", _comp.emitDir);

		a_ar.Field("baseScale", _comp.baseScale);
		a_ar.Field("burstScale", _comp.burstScale);
		a_ar.Field("burstTime", _comp.burstTime);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_context.pData);

		ImGui::SeparatorText("Mount");
		ImGui::TextDisabled("このエンティティの行列基準。エフェクトの置き方だけを決める");
		ImGui::DragFloat3("PosOffset", &_comp.posOffset.x, 0.01f);
		ImGui::DragFloat3("EmitDir (local)", &_comp.emitDir.x, 0.01f);

		ImGui::SeparatorText("Burst");
		ImGui::TextDisabled("点火した瞬間だけ大きく見せて、時間で元の大きさへ戻す");
		ImGui::DragFloat("BaseScale", &_comp.baseScale, 0.01f, 0.0f);
		ImGui::DragFloat("BurstScale", &_comp.burstScale, 0.01f, 0.0f);
		ImGui::DragFloat("BurstTime (s)", &_comp.burstTime, 0.01f, 0.0f);
		if (_comp.burstTime <= 0.0f)
		{
			ImGui::TextDisabled("0 : 膨らませない(常に BaseScale)");
		}

		// ランタイムは表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("BurstTimer : %.3f", _comp.burstTimer);
		ImGui::Text("Playing    : %s", _comp.wasPlaying ? "true" : "false");
	}
};
