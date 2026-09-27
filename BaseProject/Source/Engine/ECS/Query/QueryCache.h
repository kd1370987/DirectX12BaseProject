#pragma once

#include "../Archetype/Archetype.h"

namespace Engine::ECS
{
	struct Chunk;

	//==========================================================================================
	// クエリ結果のキャッシュ
	//
	// 条件に一致したアーキタイプと、そこに属するチャンクの一覧を持つ。
	//   ・アーキタイプ : 消えないので、増えた分だけ照合して足す(checkedArchetypeCount まで見た)
	//   ・チャンク     : 増減したら(世代が進んだら)一致済みのアーキタイプから並べ直す。照合はしない
	// 弾が出てチャンクが1つ増えただけで、全タスクがシグネチャの照合をやり直すことはない。
	//
	// 中身の更新は World::ResolveQuery が行う。シグネチャは照合のたびに組むので、
	// 型の登録がタスクの登録より後になっても拾える。
	//==========================================================================================
	struct QueryCache
	{
		// まだ一度もクエリしていない
		static constexpr uint64_t INVALID_GENERATION = UINT64_MAX;

		std::vector<Chunk*>		chunkVec				= {};					// 条件に一致したチャンク
		std::vector<Archetype*>	archetypeVec			= {};					// 条件に一致したアーキタイプ(生成順)
		uint32_t				checkedArchetypeCount	= 0;					// 照合を済ませたアーキタイプの数(生成順の先頭から)
		uint64_t				generation				= INVALID_GENERATION;	// chunkVec を並べたときのチャンクの世代

		// 渡した世代と食い違っていたらチャンクの並べ直しが必要
		bool IsStale(uint64_t a_currentGeneration) const { return generation != a_currentGeneration; }

		//--------------------------------------------------------------------------------------
		// 2つのクエリの対象が重なりうるか : 同じアーキタイプに一致していれば重なる
		//
		// エンティティはちょうど1つのアーキタイプに属するので、一致したアーキタイプが
		// 1つも共通でなければ、2つのクエリが同じエンティティを回すことはない。
		// 照合の進み具合が違う・まだクエリしていないものは、分からないので重なる扱い
		//--------------------------------------------------------------------------------------
		static bool IsOverlap(const QueryCache& a_lhs, const QueryCache& a_rhs)
		{
			if (a_lhs.generation == INVALID_GENERATION || a_rhs.generation == INVALID_GENERATION) return true;
			if (a_lhs.checkedArchetypeCount != a_rhs.checkedArchetypeCount) return true;

			// どちらも生成順に並んでいるので、突き合わせは一巡でよい
			auto _lit = a_lhs.archetypeVec.begin();
			auto _rit = a_rhs.archetypeVec.begin();
			while (_lit != a_lhs.archetypeVec.end() && _rit != a_rhs.archetypeVec.end())
			{
				if (*_lit == *_rit) return true;
				if ((*_lit)->index < (*_rit)->index) ++_lit; else ++_rit;
			}
			return false;
		}
	};
}
