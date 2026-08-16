#pragma once

//==========================================================================================
//
// プレハブを座標指定で生成するための小さなヘルパー。
//
// 「爆発プレハブを当たった場所に出す」処理が、当たって消える側(ExplodeOnHitSystem)と
// 体力が尽きて消える側(HealthSystem)の2箇所に必要になったのでまとめている。
// ウェーブでの敵出現(SceneSequence)も同じ経路を通る。
//
// 生成はシステム反復中に即時に行えない(アーキタイプが壊れる)ため、
// World の遅延生成コマンドに積む。実体化は次の BeginFrame。
//
//==========================================================================================

namespace Engine
{
	namespace ECS { class World; }
	namespace Resource { class ResourceManager; class Prefab; }
}

namespace App::Utility
{
	/// <summary>
	/// 生成時に上書きする初期値。
	/// LocalTransform を持たないプレハブでも、位置を入れるために足してから生成する。
	/// </summary>
	struct SpawnParams
	{
		DirectX::XMFLOAT3 pos  = { 0.0f, 0.0f, 0.0f };			// 生成位置(ワールド)

		// 生成時の向き。isOverrideRotation が false ならプレハブの保存値をそのまま使う
		// (爆発エフェクトのように、プレハブ側で付けた傾きを潰したくない用途があるため)
		DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool isOverrideRotation = false;

		// 生成元の印(SpownerComponent)。付けると出した側が生存数を数えられる。
		// spownerGUID が無効なら印は付けない。
		Engine::GUID spownerGUID = Engine::DefaultGUID;
		int          waveIndex   = -1;
	};

	/// <summary>
	/// プレハブを生成する(遅延コマンドを積むだけ)
	/// </summary>
	/// <param name="a_prefabGUID">生成するプレハブ。未設定なら何もしない</param>
	/// <param name="a_refHandle">解決済みハンドルの置き場。未解決ならここへ解決結果を書き戻す</param>
	/// <param name="a_params">位置・向き・生成元の印</param>
	/// <returns>生成コマンドを積めたら true(プレハブが引けなければ false)</returns>
	bool SpawnPrefab(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const SpawnParams& a_params);

	/// <summary>
	/// プレハブを指定座標に生成する(向きと印は既定のまま)
	/// </summary>
	bool SpawnPrefabAt(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const DirectX::XMFLOAT3& a_pos);
}
