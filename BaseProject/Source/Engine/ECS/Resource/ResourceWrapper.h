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
	};
}