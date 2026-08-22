#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorHelper.h"
#include "Engine/ECS/World/World.h"

//==========================================================================================
// EffectAssetComponent
//
// パーティクルとメッシュをまとめた EffectAsset を再生するコンポーネント。
//
// ParticlesComponent が「1つのパーティクルアセットを出す」のに対して、
// こちらは「ジェット噴射」「爆発」といった演出ひとまとまりを1枚のアセットで扱う。
// 制御側のシステムは isPlay を書くだけでよく、
// 何個のパーティクルとメッシュで出来ているかを知らなくてよい。
//
// アセットは全員で共有する設計図なので、進行状態は instance が持つ
// (解決 : EffectFixupSystem / 進行 : EffectUpdateSystem / 発生・描画 : EffectDrawSystem)。
//==========================================================================================
struct EffectAssetComponent
{
	// エフェクトアセットのGUID(保存されるのはこれと playOnStart だけ)
	Engine::GUID effectGUID = Engine::DefaultGUID;

	// GUIDから解決したアセットのハンドル
	Engine::Handle<Engine::Resource::EffectAsset> effectHandle = {};

	// このエンティティ専用の進行状態
	Engine::Resource::EffectInstance instance = {};

	// 生成された時点から再生するか。
	// 爆発のように出しっぱなしで完結するものはこれを立てる。
	// 噴射のように状況で入り切りするものは false にして、制御側が isPlay を書く
	bool playOnStart = false;

	// 全パーツを出し終わったら自分ごと消えるか。
	// 単発エフェクトのプレハブに立てておくと、後片付けが要らなくなる
	// (出しっぱなしのパーツが1つでもあると終わらないので、その場合は何も起きない)
	bool destroyOnFinish = false;

	// ---- ランタイム(保存しない) ----
	// 再生させたいか。制御側のシステムが毎フレーム書く
	bool isPlay = false;

	//======================================================================================
	// 出す側からの上書き(ランタイム。制御側のシステムが毎フレーム書く)
	//
	// アセットは GUID 単位で全員に共有されるので、「同じ噴射でも取り付け位置と向きが
	// 個体ごとに違う」といった差はアセットへは書けない。かといって個体ごとに
	// アセットを増やすと、絵を1つ直すのに全部開いて回ることになる。
	// そこで「アセットが決めるのは中身、個体が決めるのは置き方」に分け、
	// 置き方はここへ毎フレーム書いてもらう(BoosterEffectSystem など)。
	//======================================================================================

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
struct Engine::ECS::ComponentTraits<EffectAssetComponent>
{
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

		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Change Effect",
			"EffectAsset",
			_comp.effectGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる。
			// プレハブ編集では実体が無く entity は INVALID なので、
			// GUID の書き換えだけ行い、リフレッシュはしない(無効IDで参照するとレンジ外になる)
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->AddRefreshEntity(a_context.entity);
			}
		}

		// 出っぱなしにするか。切り替えは即座に反映して、エディタで確認できるようにする
		// (生成時の反映は EffectFixupSystem が行う)
		if (ImGui::Checkbox("PlayOnStart", &_comp.playOnStart))
		{
			_comp.isPlay = _comp.playOnStart;
		}
		ImGui::Checkbox("DestroyOnFinish", &_comp.destroyOnFinish);
		ImGui::TextDisabled("出し切ったら自分ごと消す(出しっぱなしのパーツがあると消えない)");

		if (_comp.effectGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : 何も出ない)");
			return;
		}

		// 中身の確認用。細かい編集はアセット側のインスペクターで行う
		auto* _pEffect = Engine::Resource::ResourceManager::Instance().Ref(_comp.effectHandle);
		if (!_pEffect)
		{
			ImGui::TextDisabled("(読み込み中)");
			return;
		}

		ImGui::Separator();
		ImGui::Text("Particle Parts : %d", static_cast<int>(_pEffect->GetParticleParts().size()));
		ImGui::Text("Mesh Parts     : %d", static_cast<int>(_pEffect->GetMeshParts().size()));

		ImGui::Separator();
		ImGui::Text("Runtime");
		ImGui::Checkbox("IsPlay", &_comp.isPlay);
		ImGui::Text("Elapsed : %.2f", _comp.instance.elapsed);

		// 置き方の上書き : 制御側のシステムが毎フレーム書くので表示だけ
		ImGui::Text("Scale : %.2f", _comp.effectScale);
		ImGui::Text("LengthScale : %.2f", _comp.effectLengthScale);
		if (_comp.isOverrideTransform)
		{
			ImGui::Text("Override Pos : %.2f, %.2f, %.2f",
				_comp.overridePosOffset.x, _comp.overridePosOffset.y, _comp.overridePosOffset.z);
			ImGui::Text("Override Dir : %.2f, %.2f, %.2f",
				_comp.overrideEmitDir.x, _comp.overrideEmitDir.y, _comp.overrideEmitDir.z);
		}
		else
		{
			ImGui::TextDisabled("(置き方はアセットのパーツ側)");
		}
	}
};
