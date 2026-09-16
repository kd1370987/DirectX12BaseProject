#pragma once

//==========================================================================================
// SwarmBossBoidTag
//
// 群れのボス(SwarmBossController)の体を作っているボイドの印。
//
// ・付けるのは生成した SwarmBossController。プレハブには入れない
//   (同じボイドのプレハブを別の用途で使っても、ボスの体として数えられないようにするため)。
// ・Controller が毎フレームこの印を数え、その数をボスの体力として扱う。
//   ボイドが撃ち落とされれば数が減り、それがそのままボスのダメージになる。
// ・数えるだけの印なので中身は持たない(保存もしない)。
//   どのボスのものかは一緒に付く SpawnerComponent のGUIDで見分ける。
//==========================================================================================
struct SwarmBossBoidTag {};

template<>
struct Engine::ECS::ComponentTraits<SwarmBossBoidTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};
