#pragma once
namespace Engine::Option
{
	enum class EOptionCategory
	{
		None,
		Graphics,
		Count
	};

	struct IOption
	{
		IOption() = default;
		virtual ~IOption() = default;

		// 名前取得
		virtual const std::string& GetName() = 0;

		// カテゴリー取得
		virtual EOptionCategory GetCategory() = 0;

		// エディターでの表示
		virtual void DrawEdit() = 0;

		// 保存処理
		virtual void Archive(Persistence::Archive&) = 0;
	};
}