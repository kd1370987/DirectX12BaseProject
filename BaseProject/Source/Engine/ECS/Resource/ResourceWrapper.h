#pragma once
namespace Engine::ECS
{
	/// <summary>
	/// カスタム型消去用ラッパー
	/// </summary>
	struct IResourceWrapper
	{
		// デストラクタは仮想関数にして派生クラスのデストラクタを呼ばせるようにする
		virtual ~IResourceWrapper() = default;

		// 型の情報(プロファイラ用)
		virtual std::string_view GetTypeName() const = 0;	// 型名
		virtual size_t GetTypeSize() const = 0;				// sizeof
	};

	template<typename T>
	struct ResourceWrapper final : public IResourceWrapper
	{
		T data;

		/// <summary>
		/// コンストラクタ : 
		/// ItemPool や RagePool などを入れる想定
		/// </summary>
		template<typename... Args>
		ResourceWrapper(Args... a_args) : data(std::forward<Args>(a_args)...) {}

		std::string_view GetTypeName() const override { return TypeInfo::GetTypeName<T>(); }
		size_t GetTypeSize() const override { return sizeof(T); }
	};
}