#pragma once

#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// SerchGroundComponent
//
// 自分の真上と真下にレイを打って、地面との関係を把握する
//
// ・レイを打つのは SerchGroundSystem(Update)。ここに入る結果は毎フレーム書き直すので保存しない。
// ・地面の中に居るかは「真上のレイが上向きの面の裏に当たったか」で見る。
//   地面の下から上へ打つと地表の裏に当たり、その面の法線は上(レイと同じ向き)を向く。
//   地上に居て真上に岩などがあっても、そちらは下向きの面なので地中とは見なさない。
// ・地面として見るのは StaticObject のレイヤーだけ(ボイドやプレイヤーを地面と取り違えない)。
// ・今はワームボスのリーダーが使う(アッパー攻撃で潜る深さを決める)。
//   付けるのは SwarmBossController。プレハブに入れ忘れていても生成時に足される。
//==========================================================================================
struct SerchGroundComponent
{
	float maxDistance = 200.0f;			// 上下それぞれのレイの長さ(保存される)

	//------------------------------------------------------------------------------------------
	// 結果(SerchGroundSystem が書く。保存しない)
	//------------------------------------------------------------------------------------------
	Engine::ECS::Flg isFoundGround = 0;	// 上下どちらかのレイで地面が見つかったか
	Engine::ECS::Flg isUnderGround = 0;	// 地面の中に居るか
	float groundHeight = 0.0f;			// 真上か真下の地表の高さ(ワールド。見つかったときだけ有効)
};

template<>
struct Engine::ECS::ComponentTraits<SerchGroundComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SerchGroundComponent& _comp = Engine::Editor::GetValue<SerchGroundComponent>(a_pData);
		a_ar.Field("maxDistance", _comp.maxDistance);
	}

	static void Edit(CompEditContext& a_context)
	{
		SerchGroundComponent& _comp = Engine::Editor::GetValue<SerchGroundComponent>(a_context.pData);
		Engine::Editor::Field("MaxDistance", _comp.maxDistance, 1.0f, 0.0f);

		// 毎フレーム書き直される値なので表示のみ
		if (!_comp.isFoundGround)
		{
			Engine::Editor::HelpText("Ground : (not found)");
			return;
		}
		Engine::Editor::Value("Ground", "%.1f (%s)", _comp.groundHeight, _comp.isUnderGround ? "under" : "above");
	}
};
