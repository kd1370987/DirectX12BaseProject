#pragma once
namespace Engine::ECS
{
	struct EngineServices;
}

namespace Engine::Option
{
	enum class EOptionCategory
	{
		None,
		Graphics,
		Project,
		Debug,
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
		// a_services : アセットを選ばせる項目(カーソル画像など)が一覧を引く先
		virtual void DrawEdit(const ECS::EngineServices& a_services) = 0;

		// 保存処理
		virtual void Archive(Persistence::Archive&) = 0;
	};
}