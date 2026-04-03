#pragma once

#include "../Internal/SystemComon.h"

#include "../System/ISystem/ISystem.h"

namespace Engine::ECS
{

	class World;

	class SystemManager
	{
	public:

		void Init();

		/// <summary>
		/// システムの登録
		/// </summary>
		/// <typeparam name="System">システム型</typeparam>
		template<typename System>
		void Register();

		/// <summary>
		/// システムの更新
		/// </summary>
		/// <param name="a_world">ワールドの参照</param>
		/// <param name="a_type">更新させたいシステムタイプ</param>
		/// <param name="a_dt">デルタタイム</param>
		void RunSystem(
			World& a_world, const ESystemType& a_type, float a_dt
		);

		/// <summary>
		/// トポロジカルソート、システムの依存関係を解決する
		/// </summary>
		void Sort();	// 未実装

	private:

		// 登録されているシステム群
		std::unordered_map<ESystemType, std::vector<std::shared_ptr<ISystem>>> m_systemMap;
	};

	template<typename System>
	inline void SystemManager::Register()
	{
		//static_assert(std::is_base_of_v<ISystem, System>, "ISystemを継承していません");
		std::shared_ptr<ISystem> _spSys = std::make_shared<System>();

		auto _it = m_systemMap.find(System::s_type);
		if (_it != m_systemMap.end())
		{
			m_systemMap[System::s_type].push_back(_spSys);
		}
		else
		{
			ImGuiContex::Instance().AddLog("登録済みシステムです");
		}
	}

}