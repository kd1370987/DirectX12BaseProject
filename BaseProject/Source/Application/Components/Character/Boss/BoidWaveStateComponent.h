#pragma once

//==========================================================================================
// BoidWaveStateComponent
//
// ワームの体(ボイド)を走る発光ウェーブの計算途中の値。書くのは BoidWaveSystem だけ。
//
// ・以前は BoidComponent::distanceFromPlatoonLeader に置いていた。
//   BoidComponent は操舵の設定と所属(小隊長)を持つもので、BoidSystem / SwarmLookSystem が読む。
//   そこへ BoidWaveSystem が書き込むと、読むだけの側まで書き手とぶつかり、
//   Update フェーズの依存が循環していた(ソートが失敗していた)。
// ・付けるのは SwarmBossController(ボイドの生成時)。プレハブには入れない。
// ・実行中の値だけなので保存しない。
//==========================================================================================
struct BoidWaveStateComponent
{
	// 小隊長からの距離 : 一次元距離
	//
	// 小隊長から見た実際の位置を、その進行方向へ投影したもの(頭側が負、尾側が正)。
	// 小隊長の distanceAlongWorm に足して「ワームの頭から何m地点に居るか」を出す。
	// 発光のウェーブはその位置で決まる
	float distanceFromPlatoonLeader = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<BoidWaveStateComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		BoidWaveStateComponent& _comp = Engine::Editor::GetValue<BoidWaveStateComponent>(a_context.pData);

		// 毎フレーム計算される値なので表示のみ
		Engine::Editor::Value("FromPlatoonLeader", "%.1f m", _comp.distanceFromPlatoonLeader);
	}
};
