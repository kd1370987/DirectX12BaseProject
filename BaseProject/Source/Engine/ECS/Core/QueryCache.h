#pragma once
namespace Engine::ECS
{
	struct Chunk;

	//==========================================================================================
	// クエリ結果のキャッシュ
	//
	// 条件に一致するチャンクの一覧と、それを作ったときのアーキタイプの世代を持つ。
	// 世代が進んでいたら(チャンクが増えていたら)作り直す。
	//
	// 中身の更新は World::ResolveQuery が行う。シグネチャは作り直しのたびに組むので、
	// 型の登録がタスクの登録より後になっても拾える。
	//==========================================================================================
	struct QueryCache
	{
		// まだ一度もクエリしていない
		static constexpr uint64_t INVALID_GENERATION = UINT64_MAX;

		std::vector<Chunk*>	chunkVec	= {};					// 条件に一致したチャンク
		uint64_t			generation	= INVALID_GENERATION;	// chunkVec を作ったときの世代

		// 渡した世代と食い違っていたら作り直しが必要
		bool IsStale(uint64_t a_currentGeneration) const { return generation != a_currentGeneration; }
	};
}
