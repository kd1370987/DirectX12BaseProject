#pragma once

namespace Engine::ECS
{
	struct Chunk;

	// エンティティの住所
	struct EntityLocation
	{
		Chunk*		pChunk = nullptr;
		uint32_t	chunkIndex = 0;
	};
}
