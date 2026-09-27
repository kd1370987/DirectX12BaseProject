#pragma once

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
	// エフェクト全体のスケール倍率。1 で等倍。
	// パーティクルの粒の大きさ・ばらつき半径・パーツの配置とメッシュにまとめて掛かる
	float effectScale = 1.0f;

	// 噴射の「長さ」の倍率。1 で等倍。
	// 粒の初速に掛かるので、寿命が同じなら飛ぶ距離＝束の長さがそのまま倍率ぶん伸びる。
	// effectScale と分けてあるのは、太さは変えずに長さだけ伸ばしたい場面
	// (チャージダッシュの撃ち出しなど)があるため。
	// あちらを上げると粒もばらつき半径も一緒に太るので、噴射が丸く膨らんでしまう
	float effectLengthScale = 1.0f;

	// 下の2つを使うか。false ならアセットのパーツが持っている値をそのまま使う
	bool isOverrideTransform = false;

	Math::Vector3 overridePosOffset = { 0.0f, 0.0f, 0.0f };	// オーナーの行列基準の発生位置
	Math::Vector3 overrideEmitDir = { 0.0f, 0.0f, 1.0f };	// オーナーの行列基準の発生方向
};

template<>
struct Engine::ECS::ComponentTraits<EffectOverrideComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		EffectOverrideComponent& _comp = Engine::Editor::GetValue<EffectOverrideComponent>(a_context.pData);

		// 制御側のシステムが毎フレーム書くので表示だけ
		Engine::Editor::Value("Scale", "%.2f", _comp.effectScale);
		Engine::Editor::Value("LengthScale", "%.2f", _comp.effectLengthScale);
		if (_comp.isOverrideTransform)
		{
			Engine::Editor::Value("Override Pos", "%.2f, %.2f, %.2f", _comp.overridePosOffset.x, _comp.overridePosOffset.y, _comp.overridePosOffset.z);
			Engine::Editor::Value("Override Dir", "%.2f, %.2f, %.2f", _comp.overrideEmitDir.x, _comp.overrideEmitDir.y, _comp.overrideEmitDir.z);
		}
		else
		{
			Engine::Editor::HelpText("(置き方はアセットのパーツ側)");
		}
	}
};
