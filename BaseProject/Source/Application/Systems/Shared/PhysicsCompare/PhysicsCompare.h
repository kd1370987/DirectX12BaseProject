#pragma once

#include <chrono>

//==========================================================================================
// 旧(CollisionWorld)と Jolt(PhysicsWorld)の判定結果の比較
//
// 移行中だけ使う(PhysicsMigrationOption::compareQueries が立っているとき)。Phase 6 で消す。
//
// ずれを1件ずつログへ出すと、4000体のボイドの押し出しなどで毎フレーム大量に流れて
// ゲームごと重くなる(ホットパスのログは保留キューが掃けない)。
// なので詳細は最初の数件だけ、あとは数えておいて数秒おきに1行でまとめる。
//==========================================================================================
namespace App::Systems::PhysicsCompare
{
	// システムごとに1つ持つ(static で置く。ECS はシングルスレッド)
	struct Stats
	{
		const char* name = "";

		uint64_t queries = 0;		// 比べた回数
		uint64_t mismatches = 0;	// ずれた回数
		uint64_t differentTarget = 0;	// 両方当たったが相手が違った回数(ずれには数えない。弾の判定で使う)

		int detailLogged = 0;
		std::chrono::steady_clock::time_point lastReport = {};
	};

	// 詳細を出すのは最初のこの件数まで
	inline constexpr int kMaxDetailLogs = 5;

	// まとめを出す間隔
	inline constexpr std::chrono::seconds kReportInterval{ 5 };

	// 詳細を出してよいか(出すたびに1つ数える)
	inline bool ShouldLogDetail(Stats& a_stats)
	{
		return a_stats.detailLogged++ < kMaxDetailLogs;
	}

	// 間隔が空いていればまとめを1行出して数え直す
	inline void Report(Stats& a_stats)
	{
		const auto _now = std::chrono::steady_clock::now();
		if (a_stats.lastReport == std::chrono::steady_clock::time_point{})
		{
			a_stats.lastReport = _now;
			return;
		}
		if (_now - a_stats.lastReport < kReportInterval) return;
		a_stats.lastReport = _now;

		if (a_stats.queries == 0) return;

		ENGINE_LOG("[PhysicsCompare] %s : %llu queries, %llu mismatch, %llu different target",
			a_stats.name, a_stats.queries, a_stats.mismatches, a_stats.differentTarget);

		a_stats.queries = 0;
		a_stats.mismatches = 0;
		a_stats.differentTarget = 0;
	}

	// 2点が許容差の内にあるか
	inline bool IsClose(const Math::Vector3& a_a, const Math::Vector3& a_b, float a_tolerance)
	{
		return (a_a - a_b).LengthSquared() <= a_tolerance * a_tolerance;
	}
}
