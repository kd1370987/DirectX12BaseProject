#pragma once

#include "Application/ECS/World/World.h"

#include "Application/Components/Character/Weapon/Missile/MissileLockComponent.h"

//==========================================================================================
// MissileSalvo
//
// 「溜めたミサイルを1発ずつ撃ち出す」処理をまとめたヘルパー。
// プレイヤー側の MissileSalvoSystem と、ボス側の BossMissileSalvoSystem が共有する。
//
// 溜め方(誰をロックするか)は撃つ側で大きく違う。
//   プレイヤー … コンバットレティクルの円に入った敵をスクリーン座標で拾う
//   ボス       … 相手はプレイヤー1体だけなので、狙う相手は最初から決まっている
// 一方で「撃つ」ほうは、ポッドの GunStateComponent(弾・弾速・銃口ノード)を引いて
// 散らし角を付けて撃つ、というまったく同じ処理になる。片方だけ直して食い違わないよう、
// キューの消化はここへ寄せてある。
//==========================================================================================
namespace App::Systems::MissileSalvo
{
	/// <summary>
	/// 一斉射の弾を散らす向きを作る
	/// </summary>
	/// <remarks>
	/// 基準方向を軸にしたコーンの側面へ、発射順で回しながら並べる。
	/// 散らしたあとは HomingSystem が相手へ寄せるので、
	/// 「いったん広がってから食いつく」いつもの見た目になる。
	/// </remarks>
	Math::Vector3 MakeSpreadDir(
		const Math::Vector3& a_baseDir,
		float                a_spreadRad,
		int                  a_index,
		int                  a_total);

	/// <summary>
	/// 発射キュー(MissileLockComponent の fireRemain 以下)を消化する
	/// </summary>
	/// <param name="a_ctx">System コンテキスト(World / ResourceManager / dt を使う)</param>
	/// <param name="a_missile">撃つ側の一斉射データ。fireTimer / fireRemain を進める</param>
	/// <param name="a_podEntity">ミサイルポッド(GunStateComponent と WorldMatrix を持つ)</param>
	/// <param name="a_aimDir">狙いの向き(単位ベクトル)。誘導相手が居ない弾はこちらへ飛ぶ</param>
	/// <remarks>
	/// 撃てない状況(ポッドが付いていない・弾のプレハブが未設定)ではキューを捨てる。
	/// 積んだまま残すと、次に撃とうとしたときに古い分から出てしまうため。
	/// 生成は ProjectileSpawn 経由なので、実体化は次の BeginFrame。
	/// </remarks>
	void ConsumeFireQueue(
		const Engine::ECS::SystemContext& a_ctx,
		MissileLockComponent&             a_missile,
		Engine::ECS::Entity               a_podEntity,
		const Math::Vector3&              a_aimDir);
}
