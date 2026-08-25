#pragma once

#include "Engine/ECS/World/World.h"

//==========================================================================================
// HierarchyTransform
//
// 「そのエンティティの最終的なワールド行列」を親を辿って作るヘルパー。
//
// ワールド行列を作るのは本来 CommitHierarchyWorldMatrixSystem(PostUpdate)の仕事で、
// 結果は WorldMatrixComponent に入る。ただし Start フェーズはそれより前に走るので、
// Start で登録を済ませたい側(コリジョンワールドへの静的登録)は
// WorldMatrixComponent を読んでも単位行列しか取れない。
//
// かといって LocalTransformComponent だけで組むと親の変換が丸ごと抜ける。
// 実際 RegisterCollisionWorldSystem がそれをやっていて、
// 親のグループノードに位置が入っているステージ(Desert_02 の Terrain)で
// 絵と当たり判定が別の場所に出ていた。
// 親が全部原点のステージでは答えが偶然一致するので、長いあいだ表に出ていない。
//
// 継承フラグ(ETransformInheritance)の扱いを含めて
// CommitHierarchyWorldMatrixSystem と同じ計算をここに置き、
// 片方だけ直して食い違うことがないようにする。
//==========================================================================================
namespace App::Systems::HierarchyTransform
{
	/// <summary>
	/// 親を辿ってワールド行列を組み立てる
	/// </summary>
	/// <remarks>
	/// parentID は Awake の HierarchyLinkSystem が解決済みなので Start から呼べる。
	/// LocalTransformComponent を持たない/親が辿れない場合は、そこで打ち切って
	/// それまでに積んだ行列を返す(単位行列を返して原点へ飛ばすより崩れが小さい)。
	/// 万一ヒエラルキーが輪になっていても止まらないよう、辿る深さに上限を設けている。
	/// </remarks>
	Math::Matrix CalcWorldMatrix(
		Engine::ECS::World& a_world,
		Engine::ECS::Entity a_entity);
}
