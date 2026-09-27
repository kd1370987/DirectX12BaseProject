#pragma once
namespace Engine::ECS
{
	struct Chunk;
	struct Archetype;

	//==========================================================================================
	// クエリ結果のキャッシュ
	//
	// 条件に一致するチャンクとアーキタイプの一覧と、それを作ったときのアーキタイプの世代を持つ。
	// 世代が進んでいたら(チャンクが増えていたら)作り直す。
	//
	// 中身の更新は World::ResolveQuery が行う。シグネチャは作り直しのたびに組むので、
	// 型の登録がタスクの登録より後になっても拾える。
	//==========================================================================================
	struct QueryCache
	{
		// まだ一度もクエリしていない
		static constexpr uint64_t INVALID_GENERATION = UINT64_MAX;

		std::vector<Chunk*>		chunkVec		= {};					// 条件に一致したチャンク
		std::vector<Archetype*>	archetypeVec	= {};					// 条件に一致したアーキタイプ(アドレス順)
		uint64_t				generation		= INVALID_GENERATION;	// 一覧を作ったときの世代

		// 渡した世代と食い違っていたら作り直しが必要
		bool IsStale(uint64_t a_currentGeneration) const { return generation != a_currentGeneration; }

		//--------------------------------------------------------------------------------------
		// 2つのクエリの対象が重なりうるか : 同じアーキタイプに一致していれば重なる
		//
		// エンティティはちょうど1つのアーキタイプに属するので、一致したアーキタイプが
		// 1つも共通でなければ、2つのクエリが同じエンティティを回すことはない。
		// 世代が違う(どちらかが古い)・まだクエリしていないものは、分からないので重なる扱い
		//--------------------------------------------------------------------------------------
		static bool IsOverlap(const QueryCache& a_lhs, const QueryCache& a_rhs)
		{
			if (a_lhs.generation == INVALID_GENERATION || a_lhs.generation != a_rhs.generation) return true;

			// どちらもアドレス順に並んでいるので、突き合わせは一巡でよい
			auto _lit = a_lhs.archetypeVec.begin();
			auto _rit = a_rhs.archetypeVec.begin();
			while (_lit != a_lhs.archetypeVec.end() && _rit != a_rhs.archetypeVec.end())
			{
				if (*_lit == *_rit) return true;
				if (std::less<const Archetype*>{}(*_lit, *_rit)) ++_lit; else ++_rit;
			}
			return false;
		}
	};
}
