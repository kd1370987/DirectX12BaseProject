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
	// チャンクの確保・解放は ArchetypeManager が行い、ここはポインタを並べて持つだけ
	// (後でアロケーターに差し替える前提)
	//==========================================================================================
	struct Archetype
	{
		Signature signature;

		// コンポーネント配置
		std::unordered_map<ECS::ComponentTypeID, Layout> layoutMap;

		uint32_t			chunkCapacity = 0;		// チャンクが持つ最大エンティティ数
		size_t				maxAlign = 0;			// チャンク内のコンポーネントの最大アライメント
		std::vector<Chunk*> chunks = {};
	};
}
