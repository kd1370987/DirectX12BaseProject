#pragma once

#include "../Internal/SystemComon.h"
#include "../Internal/SystemContext.h"

#include "../System/ISystem/ISystem.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

namespace Engine::Thread
{
	class JobSystem;
}

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

	//==========================================================================================
	// ソート済みタスク1件
	//
	// dependencyIndices は「同じ配列の中で、自分より前にある要素の添え字」。
	// トポロジカル順に並んでいるので、前から順にジョブを作っていけば
	// 依存先のジョブは必ず作成済みになる
	//==========================================================================================
	struct CompiledSystemTask
	{
		SystemTask* pTask = nullptr;

		// 自分より先に終わっていなければならないタスク
		// 推移的にたどれる分は落としてあるので、ここに並ぶのは直接の依存だけ
		std::vector<uint32_t> dependencyIndices = {};

		// 後続がいない = このタスクが終わればその先はない
		// フェーズ全体の終わりを待つフェンスは、これらだけを見れば足りる
		bool isTerminal = true;
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

		//------------------------------------------------------------------------------------------
		// 並列実行の ON/OFF
		//
		// 既定は OFF。
		// 依存グラフが守れるのは「コンポーネント配列への読み書き」だけで、
		// システムがシグネチャの外で触るもの
		//   ・CollisionWorld への submit / クエリ
		//   ・MainEditor のデバッグ描画バッファ
		//   ・AudioManager の再生
		//   ・ResourceManager (非同期ロードが同時に書き換える)
		// はグラフからは見えず、同時に叩かれると壊れる。
		// これらを守るか、触るシステムを直列側へ寄せてから ON にすること
		//------------------------------------------------------------------------------------------
		void SetParallelEnabled(bool a_isEnabled) { m_isParallelEnabled = a_isEnabled; }
		bool IsParallelEnabled() const { return m_isParallelEnabled; }

		// ---- アクセサ ----
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap() const;

	private:

		// フェーズ1つぶんの依存グラフを組み立てる
		void CompilePhase(
			ESystemType a_systemType,
			const std::vector<std::unique_ptr<SystemTask>>& a_taskStorage
		);

		// 読み書きの依存 : a_task が a_other の書いたものを読むか
		// 「書く側が先」と向きが一意に決まる依存
		static bool HasReadWriteDependency(const SystemTask& a_task, const SystemTask& a_other);

		// 書き込み同士の衝突 : 同じコンポーネントを両方が書くか
		// 向きは決まらないので、呼び出し側が実行順で決める
		static bool HasWriteConflict(const SystemTask& a_task, const SystemTask& a_other);

		// 並列で回してよいフェーズか
		static bool IsParallelPhase(ESystemType a_systemType);

		// 実行本体
		void RunSerial(const ESystemType& a_type, const SystemContext& a_context);
		bool RunParallel(const ESystemType& a_type, const SystemContext& a_context);

	private:

		// 登録されているシステム実体(寿命の保持のみ)
		std::vector<std::shared_ptr<ISystem>> m_systemVec;

		// 登録されているタスク
		//
		// ソート結果は SystemTask* で持つので、実体のアドレスが動くと壊れる。
		// 遅延初期化のシステムが実行中にタスクを積む経路があり、
		// vector<SystemTask> のままだと push_back の再確保で
		// ソート済みのポインタがぶら下がるため、実体を個別に確保する
		std::unordered_map<ESystemType, std::vector<std::unique_ptr<SystemTask>>> m_systemTaskMap = {};

		// ソート後のタスク : 直列実行とデバッグ表示用の並び
		std::unordered_map<ESystemType, std::vector<SystemTask*>> m_compileTaskMap = {};

		// ソート後のタスク + 依存情報 : 並列実行用
		std::unordered_map<ESystemType, std::vector<CompiledSystemTask>> m_compileGraphMap = {};

		//------------------------------------------------------------------------------------------
		// 並列実行時の作業領域
		// 毎フレーム確保し直さないよう使い回す。
		// 触るのはメインスレッドの RunSystem だけ(入れ子で呼ばないこと)
		//------------------------------------------------------------------------------------------
		std::vector<Thread::Job*> m_jobBuffer = {};			// タスクごとに作ったジョブ
		std::vector<Thread::Job*> m_dependencyBuffer = {};	// PushJob へ渡す先行ジョブ
		std::vector<Thread::Job*> m_terminalBuffer = {};	// 後続のないジョブ

		// 変更があるかどうか
		bool m_isChange = false;

		// 並列実行するか : 既定は OFF (理由は SetParallelEnabled のコメント)
		bool m_isParallelEnabled = false;
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
