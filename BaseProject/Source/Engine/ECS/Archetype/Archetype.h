#pragma once
namespace Engine::ECS
{
	struct Chunk;

	// チャンク内でのコンポーネント配列の置き場所
	struct Layout
	{
		size_t offset;				// 先頭バイト
		size_t stride;				// 一つ一つのサイズ
	};

	//==========================================================================================
	// アーキタイプ
	//
	// 「同じコンポーネントの組み合わせ」を表す。
	// レイアウトと容量はここで1度だけ計算し、所属するチャンクはすべてそれに従う。
	// チャンクの確保・解放は ArchetypeManager が(ChunkAllocator から借りて)行い、
	// ここはポインタを並べて持つだけ。
	//
	// 一度作ったアーキタイプはワールドが消えるまで消えない(生成順の番号がそのまま使える)
	//==========================================================================================
	struct Archetype
	{
		// 型IDの表で「持っていない」を表す値
		static constexpr uint16_t INVALID_LAYOUT_INDEX = UINT16_MAX;

		Archetype() { layoutIndexTable.fill(INVALID_LAYOUT_INDEX); }

		Signature signature;
		uint32_t index = 0;							// 生成順(ArchetypeManager の並びの位置)

		// コンポーネント配置 : チャンク内の並び順(タイプID順)
		std::vector<std::pair<ECS::ComponentTypeID, Layout>> layoutVec;

		// タイプID → layoutVec の添え字。RefData / GetComponentArray のたびに引くので、
		// ハッシュを引かずに添え字1回で届くよう平らな表で持つ
		std::array<uint16_t, Limits::MAX_COMPONENT_TYPES> layoutIndexTable;

		uint32_t			chunkCapacity = 0;		// チャンクが持つ最大エンティティ数
		size_t				maxAlign = 0;			// チャンク内のコンポーネントの最大アライメント
		std::vector<Chunk*> chunks = {};

		// エンティティ数が 0 のチャンク : 一つは確保しておいて、次に来たら今の分は解放する。
		// chunks にも入ったままで、エンティティが入れば空きではなくなるので nullptr に戻す
		Chunk* pFreeChunk = nullptr;

		// コンポーネントの置き場所 : 持っていなければ nullptr
		const Layout* FindLayout(ComponentTypeID a_typeID) const
		{
			if (!IsValidTypeID(a_typeID)) return nullptr;

			const uint16_t _index = layoutIndexTable[a_typeID];
			if (_index == INVALID_LAYOUT_INDEX) return nullptr;

			return &layoutVec[_index].second;
		}
	};
}
