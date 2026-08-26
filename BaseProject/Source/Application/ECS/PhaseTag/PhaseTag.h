#pragma once
//==========================================================================================
//
// ライフサイクルのフェーズタグ
//
// エンティティが「今どの段階にいるか」を表すタグ。App::ECS::World が
// 毎フレームこの順に張り替えていく。
//
//   PostDeserializeTag : 保存データから復元した直後。GUIDやハンドルの貼り直し
//   AwakeTag           : 参照の解決。必要なリソースが届くまでここで待つ
//   StartTag           : 領域確保などの一度きりの初期化
//   ActiveTag          : 通常運転
//   ReleaseTag         : 消える前の後始末(借りているものを返す)
//
// ---- 基盤(Engine::ECS)との関係 ----
// フェーズは ECS の仕組みではなくゲーム側の決めごとなので、基盤はこの5つを知らない。
// 基盤へ伝えているのは IsQueryOnlyTag の特殊化だけで、これは
// 「絞り込みには使うが、実行順を決める依存(read/write)には数えない型」の宣言。
//
// これを宣言しないと、ActiveTask が先頭に ActiveTag を非constで足すために
// 「全ての ActiveTask が ActiveTag の書き手」になり、ActiveTag を読む
// ActiveCustomTask との間で循環が成立してトポロジカルソートが失敗する。
//
//==========================================================================================

#include "../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../Components/Tag/SystemPhaseTag/AwakeTag.h"
#include "../../Components/Tag/SystemPhaseTag/StartTag.h"
#include "../../Components/Tag/SystemPhaseTag/ActiveTag.h"
#include "../../Components/Tag/SystemPhaseTag/ReleaseTag.h"

namespace Engine::ECS
{
	// 依存に数えない型として基盤へ宣言する
	template<> struct IsQueryOnlyTag<PostDeserializeTag>	: std::true_type {};
	template<> struct IsQueryOnlyTag<AwakeTag>			: std::true_type {};
	template<> struct IsQueryOnlyTag<StartTag>			: std::true_type {};
	template<> struct IsQueryOnlyTag<ActiveTag>			: std::true_type {};
	template<> struct IsQueryOnlyTag<ReleaseTag>			: std::true_type {};
}
