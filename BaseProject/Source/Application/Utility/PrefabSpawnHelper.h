#pragma once

//==========================================================================================
//
// プレハブを座標指定で生成するための小さなヘルパー。
//
// 「爆発プレハブを当たった場所に出す」処理が、当たって消える側(ExplodeOnHitSystem)と
// 体力が尽きて消える側(HealthSystem)の2箇所に必要になったのでまとめている。
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
	/// プレハブを指定座標に生成する(遅延コマンドを積むだけ)
	/// </summary>
	/// <param name="a_prefabGUID">生成するプレハブ。未設定なら何もしない</param>
	/// <param name="a_refHandle">解決済みハンドルの置き場。未解決ならここへ解決結果を書き戻す</param>
	/// <param name="a_pos">生成位置(ワールド)</param>
	void SpawnPrefabAt(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const DirectX::XMFLOAT3& a_pos);
}
