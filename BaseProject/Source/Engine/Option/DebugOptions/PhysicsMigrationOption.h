#pragma once

#include "../IOption.h"

namespace Engine::Option::DebugOptions
{
	//==========================================================================================
	// 当たり判定を自作(CollisionWorld)から Jolt(PhysicsWorld)へ移している間の切り替え
	//
	// 判定クエリを使うシステムは、ここを見て「旧だけ / Jolt だけ / 両方」を選ぶ。
	//   useJolt = false, compare = false : 旧だけ(移行前と同じ)
	//   useJolt = false, compare = true  : 両方走らせ、結果は旧を使い、差を記録する
	//   useJolt = true,  compare = true  : 両方走らせ、結果は Jolt を使い、差を記録する
	//   useJolt = true,  compare = false : Jolt だけ
	//
	// 両方走らせるときは旧と Jolt を別々のパスで回すので、プロファイラには
	// Collision_*(旧)と Physics_*(Jolt)が並んで出る。
	// 移行が終わったらこの設定ごと消す(Phase 6)
	//==========================================================================================
	struct PhysicsMigrationOption : IOption
	{
		// 静的な相手への判定(接地レイ・押し出し)の結果に Jolt を使う
		bool useJoltStaticQueries = false;

		// 旧と Jolt を両方走らせて、差を記録する
		bool compareQueries = true;

		// 位置の差がこれ以下なら同じとみなす(m)
		float compareTolerance = 0.005f;

		const std::string& GetName() override
		{
			static const std::string _name = "PhysicsMigrationOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Debug;
		}

		void DrawEdit(const ECS::EngineServices&) override
		{
			ImGui::Checkbox("Use Jolt (static queries)", &useJoltStaticQueries);
			ImGui::Checkbox("Compare with old collision", &compareQueries);
			ImGui::DragFloat("Compare tolerance (m)", &compareTolerance, 0.0005f, 0.0f, 1.0f, "%.4f");
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("useJoltStaticQueries", useJoltStaticQueries);
			a_archive.Field("compareQueries", compareQueries);
			a_archive.Field("compareTolerance", compareTolerance);
		}

		// 旧の判定を走らせるか / Jolt の判定を走らせるか
		bool RunsOld() const { return !useJoltStaticQueries || compareQueries; }
		bool RunsJolt() const { return useJoltStaticQueries || compareQueries; }
	};
}
