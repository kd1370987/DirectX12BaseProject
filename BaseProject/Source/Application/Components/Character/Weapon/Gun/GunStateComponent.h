#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"
#include "Engine/Editor/Helper/EditorHelper.inl"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"

#include "../../../../Editor/CompEditHelper/CompEditHelper.h"

// 撃ち方
enum class EFireMode : uint32_t
{
	Auto,		// 押している間、fireRate の間隔で撃ち続ける
	Burst,		// 1回で burstCount 発を fireRate の間隔で撃ち、burstInterval あけて次のバーストへ
};

// 銃(発射体)の設定と状態を持つコンポーネント。
// 「どのプレハブを・どれくらいの初速で・どう撃つか」と、
// 「今この瞬間に撃てるのか」(連射間隔・バースト・熱)をすべてここが持つ。
//
// 持ち主は WeaponTriggerComponent に「引いているか」を書くだけで、
// 撃てる/撃てないの判断には一切関わらない。判断するのは GunShootSystem。
//
// 単発撃ち(トリガーの立ち上がりで1発)は用意していない。
// 押しっぱなしの Auto を基本にして、ミサイルのようにまとめて撃つものは Burst を使う。
struct GunStateComponent
{
	float speed = 20.0f;			// 初速

	EFireMode fireMode = EFireMode::Auto;	// 撃ち方
	float fireRate = 10.0f;			// 発射レート(1秒あたりの発射数)。
									// Auto の連射間隔、Burst ではバースト内の間隔になる
	int   burstCount = 3;			// Burst : 1バーストで撃つ数
	float burstInterval = 1.0f;		// Burst : バーストを撃ち切ってから次のバーストまでの間隔(秒)

	// 発射するプレハブ
	Engine::GUID bulletPrefabGUID = {};									// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> bulletPrefabHandle = {};	// ランタイム用(発射時に解決)

	//---------------------------------------------------------------------------
	// マズルフラッシュ
	//
	// 弾を1発撃つたびに、銃口へ単発のエフェクトを出す。
	// 弾そのものは飛んで行ってしまうので、それだけだと「撃った」手応えが銃の側に残らない。
	//
	// ・出す場所と向きは弾とまったく同じ(銃口ヌルノードの位置 / 射出方向)。
	//   弾の見た目と光る位置がずれると、撃った瞬間だけ像が二重に見える。
	// ・エフェクト側は destroyOnFinish で自分から消えるので後片付けは要らない。
	//   そのため Duration を入れた(出し切りで終わる)アセットを指定すること。
	//   出しっぱなしのパーツを含むアセットだと、撃つたびに消えないものが増えていく。
	// ・発砲音もこのエフェクトのサウンドパーツに入れておけば、
	//   銃側は「撃った」と伝えるだけで絵と音が揃う。
	//---------------------------------------------------------------------------
	Engine::GUID muzzleEffectGUID = Engine::DefaultGUID;						// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::EffectAsset> muzzleEffectHandle = {};		// EffectFixupSystem が解決する
	float muzzleEffectScale = 1.0f;	// 大きさ倍率(アセットは共有なので個体差はここで付ける)

	//---------------------------------------------------------------------------
	// オーバーヒート
	//
	// 撃つほど熱が溜まり、上限に届くと撃てなくなる。
	// 冷えて復帰しきい値を下回るまでは引き金を引いても無反応。
	//
	// 既定は無効。有効にすると弾の出方が変わるので、
	// 使いたい武器だけインスペクターで入れる(既存の武器の手応えを勝手に変えないため)
	//---------------------------------------------------------------------------
	bool  useOverheat        = false;	// 熱を使うか
	float heatPerShot        = 3.0f;	// 1発あたりに溜まる熱
	float heatLimit          = 100.0f;	// ここまで溜まるとオーバーヒート
	float heatCoolRate       = 40.0f;	// 毎秒冷える量
	float overheatCoolScale  = 0.75f;	// オーバーヒート中の冷却倍率(1未満にすると復帰が遅くなる)
	float restartHeatRatio   = 0.2f;	// 復帰する熱の割合(heatLimit 比)。0なら完全に冷えるまで撃てない

	// ---- ランタイム(保存しない) ----

	// 前回発射してからの経過時間(秒)。撃った瞬間だけ 0 に戻し、あとは毎フレーム dt を足す。
	// ステート(アクションステートや構え)が変わってもリセットしない。
	// 撃っていない間も溜まり続けるので、久しぶりに引き金を引いたら即撃てる
	float timeSinceShoot = 0.0f;

	int   burstRemain = 0;			// Burst : 今のバーストで残っている発数

	float heat       = 0.0f;		// 今の熱
	bool  isOverheat = false;		// オーバーヒート中か(復帰しきい値を下回るまで撃てない)

	UINT nullPtrNodeHash = 0;		// モデルのヌルポイント名ハッシュ値
	UINT nodeIndex = 0;				// ランタイム用ノードインデックス

	// 熱の溜まり具合(0〜1)。HUD やインスペクターの表示用
	float HeatRatio() const { return (heatLimit > 0.0f) ? (heat / heatLimit) : 0.0f; }
};

template<>
struct Engine::ECS::ComponentTraits<GunStateComponent>
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
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_pData);
		auto& _resourceManager = Engine::Resource::ResourceManager::Instance();

		_resourceManager.ReleaseHandle(_comp.muzzleEffectHandle);
		_resourceManager.ReleaseHandle(_comp.bulletPrefabHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("fireMode", _comp.fireMode);
		a_ar.Field("fireRate", _comp.fireRate);
		a_ar.Field("burstCount", _comp.burstCount);
		a_ar.Field("burstInterval", _comp.burstInterval);
		a_ar.Field("bulletPrefabGUID", _comp.bulletPrefabGUID);
		a_ar.Field("useOverheat", _comp.useOverheat);
		a_ar.Field("heatPerShot", _comp.heatPerShot);
		a_ar.Field("heatLimit", _comp.heatLimit);
		a_ar.Field("heatCoolRate", _comp.heatCoolRate);
		a_ar.Field("overheatCoolScale", _comp.overheatCoolScale);
		a_ar.Field("restartHeatRatio", _comp.restartHeatRatio);
		a_ar.Field("nullPtrNodeHash", _comp.nullPtrNodeHash);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		a_ar.Field("muzzleEffectGUID", _comp.muzzleEffectGUID);
		a_ar.Field("muzzleEffectScale", _comp.muzzleEffectScale);
	}

	static void Edit(CompEditContext& a_context)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_context.pData);

		ImGui::DragFloat("Speed", &_comp.speed, 0.1f, 0.0f);

		Engine::Editor::EditorHelper::DrawEnumCombo("FireMode", _comp.fireMode);

		// 発射レート(発/秒)。Auto は連射間隔、Burst はバースト内の間隔
		if (ImGui::DragFloat("Fire Rate", &_comp.fireRate, 0.1f, 0.01f, 1000.0f, "%.2f /s"))
		{
			if (_comp.fireRate < 0.01f) _comp.fireRate = 0.01f;
		}

		// バースト設定は Burst のときだけ効くので、それ以外は無効表示にする
		ImGui::BeginDisabled(_comp.fireMode != EFireMode::Burst);
		if (ImGui::DragInt("Burst Count", &_comp.burstCount, 1, 1, 100))
		{
			if (_comp.burstCount < 1) _comp.burstCount = 1;
		}
		if (ImGui::DragFloat("Burst Interval", &_comp.burstInterval, 0.05f, 0.0f, 60.0f, "%.2f s"))
		{
			if (_comp.burstInterval < 0.0f) _comp.burstInterval = 0.0f;
		}
		ImGui::EndDisabled();

		// 発射するプレハブの選択(アセットDBの Prefab 一覧から)
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Bullet Prefab", "Prefab", _comp.bulletPrefabGUID))
		{
			// GUIDが変わったらハンドルは作り直す(発射時に再解決)
			_comp.bulletPrefabHandle = {};
		}

		App::Editor::CompEditHelper::SelectSelfModelNode(
			a_context,
			_comp.nullPtrNodeHash,
			_comp.nodeIndex
		);

		// ---- マズルフラッシュ ----
		// 1発撃つごとに銃口へ出す単発エフェクト。位置と向きは弾と同じ
		ImGui::Separator();
		ImGui::TextDisabled("Muzzle Flash : 1発撃つごとに銃口へ出す");
		Engine::Editor::EditorHelper::DrawAssetSelectCombo<Engine::Resource::EffectAsset>(
			"Muzzle Effect",
			"EffectAsset",
			_comp.muzzleEffectGUID,
			_comp.muzzleEffectHandle);
		ImGui::DragFloat("Muzzle Effect Scale", &_comp.muzzleEffectScale, 0.01f, 0.0f);
		if (_comp.muzzleEffectGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : 撃っても何も出ない)");
		}
		else
		{
			ImGui::TextDisabled("出し切ったら自分で消えるので、Duration を入れたアセットを指定すること");
		}

		// ---- オーバーヒート ----
		ImGui::Separator();
		ImGui::Checkbox("Use Overheat", &_comp.useOverheat);

		ImGui::BeginDisabled(!_comp.useOverheat);
		ImGui::DragFloat("Heat Per Shot", &_comp.heatPerShot, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("Heat Limit", &_comp.heatLimit, 1.0f, 0.01f, 10000.0f);
		ImGui::DragFloat("Heat Cool Rate", &_comp.heatCoolRate, 0.5f, 0.0f, 10000.0f, "%.2f /s");
		ImGui::DragFloat("Overheat Cool Scale", &_comp.overheatCoolScale, 0.01f, 0.0f, 4.0f);
		ImGui::DragFloat("Restart Heat Ratio", &_comp.restartHeatRatio, 0.01f, 0.0f, 1.0f);
		ImGui::EndDisabled();

		// 上限が 0 以下だと割り算も判定も壊れるので下限で止める
		if (_comp.heatLimit < 0.01f) _comp.heatLimit = 0.01f;

		// ---- ランタイム状態(参考) ----
		ImGui::Separator();
		ImGui::TextDisabled("TimeSinceShoot : %.2f s  BurstRemain : %d",
			_comp.timeSinceShoot, _comp.burstRemain);

		if (_comp.useOverheat)
		{
			ImGui::ProgressBar(_comp.HeatRatio(), ImVec2(-1.0f, 0.0f));
			ImGui::TextDisabled("Heat : %.1f / %.1f%s",
				_comp.heat, _comp.heatLimit, _comp.isOverheat ? "  [OVERHEAT]" : "");
		}
	}
};
