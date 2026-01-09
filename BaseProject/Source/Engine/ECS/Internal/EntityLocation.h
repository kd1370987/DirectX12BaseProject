#pragma once

class ArchetypeChunk;

// エンティティの住所
struct EntityLocation
{
	ArchetypeChunk* pArchetypeChunk = nullptr;
	uint32_t chunkIndex = 0;
};