#pragma once

namespace App::InstanceResource
{
	//==========================================================================================
	// BoidSnapshotResource
	//
	// 群れの操舵に使う、そのフレームのボイドの位置と速度(小隊ごと)。
	// 作るのは BoidSnapshotSystem(メインスレッド)、読むのは BoidSteeringSystem(Job)。
	//
	// ・操舵は全員の「更新前」の値で計算する。更新順で結果が変わらないようにするため。
	// ・小隊ごとの配列は使い回す。以前は BoidSystem の中で毎フレーム
	//   unordered_map<Entity, vector> を作り直していて、体数ぶんの確保が毎フレーム走っていた。
	//   今は小隊や体数が増えたときだけ確保する。
	// ・Job からは読むだけなので守りは要らない(書くのは前段で、順序は依存で決まっている)。
	//==========================================================================================
	struct BoidSnapshotResource
	{
		// ボイド1体ぶん
		struct Entry
		{
			Math::Vector3 position = {};
			Math::Vector3 velocity = {};
		};

		// 小隊長 → platoonVec の添え字
		std::unordered_map<Engine::ECS::Entity, uint32_t> platoonIndexMap = {};

		// 小隊ごとのボイド。先頭 platoonCount 本だけが今フレームの中身で、残りは使い回し用
		std::vector<std::vector<Entry>> platoonVec = {};
		uint32_t platoonCount = 0;

		// 中身だけ捨てる(確保した領域は残す)
		void Clear()
		{
			for (uint32_t _i = 0; _i < platoonCount; ++_i)
			{
				platoonVec[_i].clear();
			}
			platoonIndexMap.clear();
			platoonCount = 0;
		}

		// 小隊へ1体足す
		void Push(Engine::ECS::Entity a_platoon, const Entry& a_entry)
		{
			auto [_it, _isInserted] = platoonIndexMap.try_emplace(a_platoon, platoonCount);
			if (_isInserted)
			{
				if (platoonCount >= platoonVec.size()) platoonVec.emplace_back();
				++platoonCount;
			}
			platoonVec[_it->second].push_back(a_entry);
		}

		// 小隊のボイド一覧 : 居なければ空
		std::span<const Entry> Find(Engine::ECS::Entity a_platoon) const
		{
			auto _it = platoonIndexMap.find(a_platoon);
			if (_it == platoonIndexMap.end()) return {};
			return platoonVec[_it->second];
		}
	};
}
