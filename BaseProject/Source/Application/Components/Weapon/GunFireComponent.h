#pragma once

namespace App::Component
{
	//==========================================================================================
	// GunFireComponent
	//
	// このフレームに撃ったか、撃ったなら銃口の位置と向き。
	//
	// ・書くのは GunTriggerSystem(発射判定。Job)だけで、毎フレーム全部書き直す。
	//   読むのは GunProjectileSpawnSystem(弾の生成)と MuzzleFlashSystem(銃口の光)。
	// ・以前の GunShootSystem は判定・生成・マズルフラッシュを1つで回していた。
	//   判定は自分の銃の値だけで決まるので、ここで分けてチャンク並列に回し、
	//   生成(構造変更の予約)と再生(エフェクト)だけをメインスレッドに残している。
	//   共有の配列(Resource)へ積まず銃ごとに持つのは、Job から同時に積むと取り合いになるため。
	// ・GunStateComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
	//==========================================================================================
	struct GunFireComponent
	{
		bool isFired = false;								// このフレームに撃ったか

		Math::Vector3 spawnPos = { 0.0f, 0.0f, 0.0f };		// 弾を出す位置(ワールド)
		Math::Vector3 shootDir = { 0.0f, 0.0f, 1.0f };		// 射出方向(ワールド、単位ベクトル)
		Math::Vector3 muzzleLocalPos = { 0.0f, 0.0f, 0.0f };	// 銃ローカルの銃口位置(マズルフラッシュ用)
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::GunFireComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::GunFireComponent& _comp = Engine::EditorField::GetValue<App::Component::GunFireComponent>(a_context.pData);
		Engine::EditorField::Value("Fired", "%s", _comp.isFired ? "true" : "false");
	}
};
