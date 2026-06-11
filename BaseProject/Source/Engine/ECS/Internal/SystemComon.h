#pragma once

namespace Engine::ECS
{
	// システムの種類
	enum class ESystemType
	{
		// 初期化
		Init,
		PostDeserialize,
		Awake,
		Start,

		// 更新
		Input,
		PreUpdate,
		Update,
		Physics,
		Animation,
		Camera,
		PostUpdate,

		// 描画
		PreDraw,
		Draw,
		PostDraw,

		// 解放
		Release,

		// システム分類総数
		Num
	};

	// システムの型
	template<typename... Systems>
	struct TypeList {};

	// システムの依存関係解決用
	template<typename System>
	struct SystemTraits
	{
		using Reads = TypeList<>;
		using Writes = TypeList<>;
	};
}