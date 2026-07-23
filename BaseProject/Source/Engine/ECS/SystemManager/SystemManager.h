#pragma once

#include "../Internal/SystemComon.h"
#include "../Internal/SystemContext.h"

#include "../System/ISystem/ISystem.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

namespace Engine::ECS
{

	class World;

	// システムの実行情報（ジョブ）を保持する
	struct SystemTask
	{
		std::string name = {};
		Signature readSig;		// 読み込みのみを行うコンポーネント
		Signature writeSig;		// 書き込みを行うコンポーネント軍
		std::function<void(const SystemContext&)> executeFunc;	// チャンク処理
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
		// システムのフェーズを指定、コンテキストを入れる
		void RunSystem(
			const ESystemType& a_type, const SystemContext& a_context
		);

		// 登録されたタスクをフェーズごとにソートする
		void Sort();

		// タスクの登録
		void AddSystemTask(
			ESystemType a_systemType,const SystemTask& a_systemTask,const std::string& a_taskName
		);

		// ---- アクセサ ----
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap() const;

	private:

		// 登録されているシステム実体(寿命の保持のみ)
		std::vector<std::shared_ptr<ISystem>> m_systemVec;

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
		static_assert(std::is_base_of_v<ISystem, System>, "ISystemを継承していません");

		// システム実体は Init でタスクを登録するだけの入れ物。
		// 実行はタスク側で行うので、ここでは寿命の保持だけする。
		// (フェーズでの分類は不要。フェーズはタスク登録の引数が持つ)
		std::shared_ptr<ISystem> _spSys = std::make_shared<System>();
		_spSys->Init(*a_world);

		m_systemVec.push_back(std::move(_spSys));
	}

}