#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorHelper.h"
#include "Engine/Editor/Helper/EditorHelper.inl"

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
// ・ブーストダッシュ(Shift)の見せ方
//     ジェットは「出ているか」しか変わらないので、歩いている時と
//     一気に加速した時が同じ絵になってしまう。そこで2つ足してある。
//       (1) ダッシュしている間はジェットを boostScale 倍に太らせる(持続)
//       (2) 踏み込んだ瞬間にスパークのエフェクトを1回だけ出す(瞬間)
//     (1) はジェットそのものの大きさなのでここが倍率を持ち、
//     (2) は絵がまったく別物なので、ジェットのアセットとは分けて
//     sparkEffectGUID に別の EffectAsset を持たせる。
//     どちらを動かすかは BoosterEffectSystem が isBoosting を見て決める
//     (isBoosting を書くのは ThrusterEffectSystem)。
//
// ・チャージダッシュ(Space長押し)の見せ方
//     溜めと撃ち出しで動かす軸を分けてある。
//       溜め   : 溜まり具合ぶんジェットを chargeScale まで太らせる(だんだん)
//       撃ち出し: 束の長さを dashLengthScale 倍にする(粒の初速に掛かる)
//     どちらも太さでやると「溜まりきった」のか「もう出た」のかが読めない。
//     溜め具合と撃ち出し中かを配るのは ThrusterEffectSystem で、
//     元を持っているのは機体側の ChargeDashComponent。
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

	// ---- ブーストダッシュ中の太らせ(設定値) ----
	// 上の膨らみに掛ける倍率。1 なら通常移動と同じ太さのまま
	float boostScale = 1.7f;
	// 倍率の行き来にかける秒数。0 なら即座に切り替わる。
	// 入り切りをそのまま出すとジェットが1フレームで跳ねるので、少しだけ均す
	float boostBlendTime = 0.08f;

	// ---- チャージダッシュ(設定値) ----
	// 溜めている間は少しずつ太らせ、撃ち出した瞬間に束を前へ伸ばす。
	//
	// 溜めを太さで、撃ち出しを長さで見せているのは、2つが同時に起きないため。
	// どちらも太さでやると「溜まりきった」のか「もう出た」のかが読み取れず、
	// 撃ち出しの側を太さでやると、ただ膨らむだけで前へ進む感じが出ない
	float chargeScale = 1.6f;		// 溜まりきったときのスケール倍率(1 なら太らない)
	float dashLengthScale = 2.0f;	// ダッシュ中の束の長さの倍率(粒の初速に掛かる)
	// 長さの行き来にかける秒数。0 なら即座に切り替わる。
	// 撃ち出しは一瞬で伸びてほしいので、太さの boostBlendTime より短めが合う
	float dashLengthBlendTime = 0.05f;

	// ---- 踏み込んだ瞬間のスパーク(設定値) ----
	// ジェットとは別のエフェクトを噴射口へ1回だけ出す。
	// 未設定なら何も出ない(ジェットの太らせだけが効く)
	Engine::GUID sparkEffectGUID = Engine::DefaultGUID;
	Engine::Handle<Engine::Resource::EffectAsset> sparkHandle = {};	// EffectFixupSystem が解決する
	float sparkScale = 1.0f;	// スパークの大きさ倍率(アセットは共有なので個体差はここで付ける)

	// ---- ランタイム(保存しない) ----
	float burstTimer = 0.0f;	// 戻るまでの残り時間
	bool  wasPlaying = false;	// 点火の立ち上がりを見るための前フレームの状態

	bool  isBoosting = false;	// ブーストダッシュ中か(ThrusterEffectSystem が毎フレーム書く)
	bool  wasBoosting = false;	// 踏み込みの立ち上がりを見るための前フレームの状態
	float boostBlend = 0.0f;	// 0 = 通常 / 1 = ブースト中。boostBlendTime で行き来する

	float chargeRate = 0.0f;	// チャージの溜まり具合 0〜1(ThrusterEffectSystem が毎フレーム書く)
	bool  isChargeDashing = false;	// チャージダッシュ中か(同上)
	float dashLengthBlend = 0.0f;	// 0 = 通常 / 1 = ダッシュ中。dashLengthBlendTime で行き来する
};

template<>
struct Engine::ECS::ComponentTraits<BoosterEffectComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事。
	// ECS がエンティティを消すとき・コンポーネントを外すとき・
	// PostDeserialize へ入り直すとき(fixup が取り直す)に必ず呼ぶ。
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_pData);
		auto& _resourceManager = Engine::Resource::ResourceManager::Instance();

		_resourceManager.ReleaseHandle(_comp.sparkHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_pData);

		a_ar.Field("posOffset", _comp.posOffset);
		a_ar.Field("emitDir", _comp.emitDir);

		a_ar.Field("baseScale", _comp.baseScale);
		a_ar.Field("burstScale", _comp.burstScale);
		a_ar.Field("burstTime", _comp.burstTime);

		a_ar.Field("boostScale", _comp.boostScale);
		a_ar.Field("boostBlendTime", _comp.boostBlendTime);

		a_ar.Field("chargeScale", _comp.chargeScale);
		a_ar.Field("dashLengthScale", _comp.dashLengthScale);
		a_ar.Field("dashLengthBlendTime", _comp.dashLengthBlendTime);

		a_ar.Field("sparkEffectGUID", _comp.sparkEffectGUID);
		a_ar.Field("sparkScale", _comp.sparkScale);
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

		ImGui::SeparatorText("Boost Dash");
		ImGui::TextDisabled("ブースト中だけジェットを太らせる(上の大きさに掛かる)");
		ImGui::DragFloat("BoostScale", &_comp.boostScale, 0.01f, 0.0f);
		ImGui::DragFloat("BoostBlendTime (s)", &_comp.boostBlendTime, 0.01f, 0.0f);
		if (_comp.boostScale <= 1.0f)
		{
			ImGui::TextDisabled("1 以下 : ブーストしても太らない");
		}

		ImGui::SeparatorText("Charge Dash");
		ImGui::TextDisabled("溜めている間は太らせ、撃ち出している間は束を前へ伸ばす");
		ImGui::DragFloat("ChargeScale", &_comp.chargeScale, 0.01f, 0.0f);
		if (_comp.chargeScale <= 1.0f)
		{
			ImGui::TextDisabled("1 以下 : 溜めても太らない");
		}
		ImGui::DragFloat("DashLengthScale", &_comp.dashLengthScale, 0.01f, 0.0f);
		ImGui::DragFloat("DashLengthBlendTime (s)", &_comp.dashLengthBlendTime, 0.01f, 0.0f);
		if (_comp.dashLengthScale <= 1.0f)
		{
			ImGui::TextDisabled("1 以下 : 撃ち出しても伸びない");
		}

		ImGui::SeparatorText("Boost Spark");
		ImGui::TextDisabled("踏み込んだ瞬間に噴射口へ1回だけ出す。ジェットとは別のアセット");
		Engine::Editor::EditorHelper::DrawAssetSelectCombo<Engine::Resource::EffectAsset>(
			"Spark Effect",
			"EffectAsset",
			_comp.sparkEffectGUID,
			_comp.sparkHandle);
		ImGui::DragFloat("SparkScale", &_comp.sparkScale, 0.01f, 0.0f);
		if (_comp.sparkEffectGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : ダッシュしても何も出ない)");
		}

		// ランタイムは表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("BurstTimer : %.3f", _comp.burstTimer);
		ImGui::Text("Playing    : %s", _comp.wasPlaying ? "true" : "false");
		ImGui::Text("Boosting   : %s", _comp.isBoosting ? "true" : "false");
		ImGui::Text("BoostBlend : %.3f", _comp.boostBlend);
		ImGui::Text("ChargeRate : %.3f", _comp.chargeRate);
		ImGui::Text("ChargeDash : %s", _comp.isChargeDashing ? "true" : "false");
		ImGui::Text("DashBlend  : %.3f", _comp.dashLengthBlend);
	}
};
