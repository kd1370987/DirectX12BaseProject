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

	//--------------------------------------------------------------------------------------
	// シグネチャの添え字として使えるタイプIDか
	//
	// 未登録の型は INVALID_COMPONENTTYPEID(=255)で返ってくるが、シグネチャは
	// MAX_COMPONENT_TYPES(=200)ビットしかないので、そのまま test / set に渡すと
	// std::out_of_range で落ちる。ビットを触る前に必ずこれを通すこと
	//--------------------------------------------------------------------------------------
	constexpr bool IsValidTypeID(ComponentTypeID a_typeID)
	{
		return a_typeID < ECS::Limits::MAX_COMPONENT_TYPES;
	}

	// 型名(ログ用)は Engine::TypeInfo::GetTypeName<T>() を使う。
	// 未登録の型はレジストリに名前が無いので、警告やエラーで型を示すときに使う

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

	//--------------------------------------------------------------------------------------
	// コンポーネントに付随する処理の登録構造体
	//
	// 必要なものだけ特殊化して書く。書かなかったものは登録されず、呼ぶ側が飛ばす。
	//   static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData);	// セーブロード
	//   static void Edit(CompEditContext& a_context);								// エディター
	//   static void Release(void* a_pData, const EngineServices& a_services);		// 借りたものを返す
	// 何も要らない(タグなど)なら特殊化自体を書かなくてよい
	//--------------------------------------------------------------------------------------
	template<typename T>
	struct ComponentTraits {};
};

