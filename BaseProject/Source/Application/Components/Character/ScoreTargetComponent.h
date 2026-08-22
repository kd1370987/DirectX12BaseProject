#pragma once

//==========================================================================================
// ScoreTargetComponent
//
// 「プレイヤーが倒す相手」であることの印と、倒したときに入るスコア。
//
// ・印としての役目
//     シーンには弾を止める建物や地形、味方の部品(ブースターなど)も置かれていて、
//     当たり判定はどれも同じように起きる。何が「戦う相手」なのかは
//     当たり判定の側からは分からないので、狙う価値のあるものにだけこれを付ける。
//     ヒットマーカー(HitEffectHUD)が反応するのもこれが付いている相手だけで、
//     壁を撃っても手応えは出ない。
//
// ・スコアを敵ごとに持たせている理由
//     強い相手ほど高い、という差はシーンの中身の話であって、
//     倒した側(プレイヤー)や集計する側が知っていることではない。
//     「自分を倒すといくら入るか」を相手が持っていれば、
//     敵を増やしてもスコア側のコードは触らずに済む。
//
// 加算するのは ScoreSystem(PostUpdate)。DeathEventResource に積まれた死亡を見て、
// これが付いている相手だったぶんだけ ScoreResource へ足す。
// 表示するのは ScoreHUD。
//==========================================================================================
struct ScoreTargetComponent
{
	// ---- 設定(保存される) ----
	int score = 100;			// 倒されたときに入るスコア

	// ---- ランタイム(保存しない) ----
	// もう加算したか。
	// 死亡はフレームをまたいで演出が続く(倒れてから releaseDelay 秒後に消える)ので、
	// 死亡イベントが二度積まれても二重に入らないようにここで止める
	bool isScored = false;
};

template<>
struct Engine::ECS::ComponentTraits<ScoreTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ScoreTargetComponent& _comp = Engine::Editor::GetValue<ScoreTargetComponent>(a_pData);
		a_ar.Field("score", _comp.score);
	}

	static void Edit(CompEditContext& a_context)
	{
		ScoreTargetComponent& _comp = Engine::Editor::GetValue<ScoreTargetComponent>(a_context.pData);

		ImGui::TextDisabled("プレイヤーが倒す相手であることの印");
		ImGui::DragInt("Score", &_comp.score, 1, 0, 1000000);
		ImGui::TextDisabled("倒されたときに入る点数");

		ImGui::Separator();
		ImGui::TextDisabled("これが付いている相手に当てたときだけ");
		ImGui::TextDisabled("ヒットマーカーが出ます");

		ImGui::Separator();
		ImGui::Text("Scored : %s", _comp.isScored ? "true" : "false");
	}
};
