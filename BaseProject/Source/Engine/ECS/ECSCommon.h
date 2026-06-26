#pragma once

namespace Engine::ECS
{
	// エンティティの情報
	using Generation = uint32_t;
	using EntityIndex = uint32_t;

	using Entity = uint64_t;

	// 型情報をビット変換
	using ComponentTypeID = uint32_t;

	// 上限・エラー数値
	namespace Limits
	{
		constexpr uint32_t MAX_ENTITIES = 100000;
		constexpr uint32_t MAX_COMPONENT_TYPES = 200;

		constexpr Entity INVALID_ENTITY = UINT64_MAX;

		constexpr ComponentTypeID INVALID_COMPONENTTYPEID = UINT8_MAX;
	}

	constexpr uint32_t ENTITY_INDEX_BITS = 32;
	constexpr uint32_t GENERATION_BITS = 32;

	// コンポーネントタイプのビットセット
	using Signature = std::bitset<ECS::Limits::MAX_COMPONENT_TYPES>;

	using Flg = uint8_t;

	// シリアライズ、デシリアライズ用関数
	using SerializeFunc = void(*)(const void*, nlohmann::json&);
	using DeserializeFunc = void(*)(void*, const nlohmann::json&);

	// コンポーネントのシリアライズ登録構造体
	template<typename T>
	struct ComponentTraits {
		// コンポーネント側で特殊化されることを期待する
		static void Archive(Engine::Persistence::Archive& a_ar, T& a_comp) = delete;
		static void Edit(T& a_comp) = delete;
	};
};

