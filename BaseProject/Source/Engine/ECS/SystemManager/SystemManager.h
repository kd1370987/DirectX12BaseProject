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

		// 初期化
		void Init();

		// システムの登録
		template<typename System>
		void Register(World* a_world);

		// システムの更新
		// システムのフェーズを指定、デルタタイムを入れる
		void RunSystem(
			World& a_world, const ESystemType& a_type, float a_dt
		);

		// 登録されたタスクをフェーズごとにソートする
		void Sort();

		// タスクの登録
		void AddSystemTask(ESystemType a_systemType,const SystemTask& a_systemTask);

	private:

		// 登録されているシステム群
		std::unordered_map<ESystemType, std::vector<std::shared_ptr<ISystem>>> m_systemMap;

		// 登録されているタスク
		std::unordered_map<ESystemType, std::vector<SystemTask>> m_systemTaskMap = {};

		// ソート後のタスク
		std::unordered_map<ESystemType, std::vector<SystemTask*>> m_compileTaskMap = {};

		// 変更があるかどうか
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