#pragma once

//==========================================================================================
// PlatoonAxisResource
//
// 小隊長ごとの「位置・進んでいる向き・頭からの1次元位置」。
// 作るのは PlatoonAxisSystem(メインスレッド)、読むのは BoidWaveSystem(Job)。
//
// ・以前は BoidWaveSystem がチャンクごとに ForEach で小隊長を集め直していた
//   (チャンク数 × 小隊長の数)。前段で1回だけ作って、ここから引く。
// ・Job からは読むだけなので守りは要らない。
//==========================================================================================
struct PlatoonAxisResource
{
	// 小隊長1体ぶん
	struct Axis
	{
		Math::Vector3 pos = {};				// 位置(ワールド)
		Math::Vector3 forward = {};			// 進んでいる向き(単位ベクトル)
		float distanceAlongWorm = 0.0f;		// 頭からの1次元位置
	};

	std::unordered_map<Engine::ECS::Entity, Axis> axisMap = {};

	void Clear() { axisMap.clear(); }

	// 小隊長の軸 : 居なければ nullptr
	const Axis* Find(Engine::ECS::Entity a_platoon) const
	{
		auto _it = axisMap.find(a_platoon);
		return (_it != axisMap.end()) ? &_it->second : nullptr;
	}
};
