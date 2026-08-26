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

	//--------------------------------------------------------------------------------------
	// 問い合わせ専用のタグかどうか
	//
	// エンティティの絞り込みにだけ使い、実行順を決める依存(read/write)には数えない
	// コンポーネントを、上位層がここを特殊化して宣言する。
	//
	// 基盤はどの型がそれに当たるかを知らない。ゲーム側のライフサイクルタグ
	// (App::ECS のフェーズタグ)がこれを true にしている。
	//--------------------------------------------------------------------------------------
	template<typename T>
	struct IsQueryOnlyTag : std::false_type {};

	template<typename T>
	inline constexpr bool IsQueryOnlyTag_v = IsQueryOnlyTag<T>::value;

	// シリアライズ、デシリアライズ用関数
	using SerializeFunc = void(*)(const void*, nlohmann::json&);
	using DeserializeFunc = void(*)(void*, const nlohmann::json&);

	// コンポーネントのシリアライズ登録構造体
	template<typename T>
	struct ComponentTraits {
		// コンポーネント側で特殊化されることを期待する
		static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) = delete;
		static void Edit(void* a_pData) = delete;
	};
};

