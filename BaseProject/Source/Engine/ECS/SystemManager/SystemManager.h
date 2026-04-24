#pragma once

#include "../Internal/SystemComon.h"

#include "../System/ISystem/ISystem.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

namespace Engine::ECS
{

	class World;

	// システムの実行情報（ジョブ）を保持する
	struct SystemTask
	{
		Signature readSig;		// 読み込みのみを行うコンポーネント
		Signature writeSig;		// 書き込みを行うコンポーネント軍
		std::function<void(float)> executeFunc;	// チャンク処理
	};


	class SystemManager
	{
	public:

		void Init();

		/// <summary>
		/// システムの登録
		/// </summary>
		/// <typeparam name="System">システム型</typeparam>
		template<typename System>
		void Register(World* a_world);

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
		void Sort();

		// タスクの登録
		void AddSystemTask(ESystemType a_systemType,const SystemTask& a_systemTask);

	private:

		// 登録されているシステム群
		std::unordered_map<ESystemType, std::vector<std::shared_ptr<ISystem>>> m_systemMap;


		std::unordered_map<ESystemType, std::vector<SystemTask>> m_systemTaskMap = {};
		std::unordered_map<ESystemType, std::vector<SystemTask*>> m_compileTaskMap = {};

		bool m_isChange = false;
	};

	template<typename System>
	inline void SystemManager::Register(World* a_world)
	{
		//static_assert(std::is_base_of_v<ISystem, System>, "ISystemを継承していません");

		std::shared_ptr<ISystem> _spSys = std::make_shared<System>();
		_spSys->Init(*a_world);

		auto _it = m_systemMap.find(System::s_type);
		if (_it != m_systemMap.end())
		{
			m_systemMap[System::s_type].push_back(_spSys);
		}
		else
		{
			// 新たなシステムタイプなので新規に増やして登録
		}
	}

}