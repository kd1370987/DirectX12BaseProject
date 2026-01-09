#pragma once

struct Layout
{
	size_t offset;
	size_t stride;
};

struct ArchetypeChunk
{
	// エンティティ
	ECS::Entity* entityData = nullptr;			// エンティティ配列
	uint32_t		capacity	= 0;			// チャンクが持つ最大エンティティ数
	uint32_t		count		= 0;			// 現在のエンティティ数

	// コンポーネント
	//std::vector<size_t> layoutVec;				// 配置レイアウト
	std::unordered_map<ECS::ComponentTypeID, Layout> layoutMap;
	size_t			maxAlign = 0;				// チャンク内のコンポーネントの最大アライメント
	uint8_t*		data		= nullptr;		// バイトデータ
};