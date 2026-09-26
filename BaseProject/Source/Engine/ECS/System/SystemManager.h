#pragma once

#include "SystemCommon.h"
#include "SystemContext.h"

#include "ISystem.h"

#include "../Component/ComponentMetaRegistry.h"
#include "../Query/QueryCache.h"

namespace Engine::ECS
{

	class World;
	class ECSWorldProfiler;

	// システムの実行方法
	enum class ETaskExec : uint8_t { MainThread, Job };

	// システムの実行情報（ジョブ）を保持する
	struct SystemTask
	{
		std::string name = {};
		Signature readSig;											// 読み込みのみを行うコンポーネント
		Signature writeSig;											// 書き込みを行うコンポーネント軍

		std::function<void(SystemTask&, const SystemContext&)> executeFunc;	// チャンク処理(自身のタスクを受け取る)
		QueryCache query;											// クエリ結果(RegisterTask のみ使う。カスタムタスクは空のまま)

		ETaskExec exec = ETaskExec::MainThread;						// システムの実行方法

		// チャンク分割で回すための口 : RegisterTaskのみ CustomTaskはnullptr
		// メインスレッドでクエリを解決後チャンク数を返す
		std::function<uint32_t(SystemTask&, const SystemContext&)> prepareFunc;

		// [begin,end)のチャンクを処理(end は含まない)
		std::function<void(SystemTask&, const SystemContext&, uint32_t, uint32_t)>executeRangeFunc;
	};

	struct CompileTask
	{
		SystemTask* pTask = nullptr;
		std::vector<uint32_t> waitIndices;		// 同じフェーズで自分より前にあり、衝突するJobタスクの並び位置
	};

	//------------------------------------------------------------------------------------------
	// 並びの診断(Sort のたびに作り直す。実行には使わない)
	//
	// ソートが辺にするのは RAW(読む側 → 書いた側の後)だけなので、
	// 書き手同士や「読んだ後に書く」組み合わせは、段と登録順で並んでいるにすぎない。
	// それを見えるようにするための記録
	//------------------------------------------------------------------------------------------

	// 衝突しているのに、依存(RAW)の経路で前後が保証されていない組
	struct ScheduleAmbiguity
	{
		const SystemTask* pEarlier = nullptr;	// 今の並びで先に走る方
		const SystemTask* pLater = nullptr;		// 今の並びで後に走る方
		Signature conflictSig;					// ぶつかっているコンポーネント
	};

	// フェーズ1つぶんの診断
	struct PhaseScheduleReport
	{
		bool isSorted = true;								// トポロジカルソートが成功したか
		std::vector<const SystemTask*> cyclicTaskVec;		// 循環に巻き込まれ、登録順で末尾に足されたもの
		std::vector<ScheduleAmbiguity> ambiguityVec;		// 前後が依存で決まっていない衝突
	};


	//==========================================================================================
	// システムの管理
	//
	// タスクはフェーズごとにソートされた順に回す。
	//   MainThread : メインスレッドでその場で実行する
	//   Job        : ワーカーへ積む。RegisterTask のものはチャンクを分けて複数のジョブで回し、
	//                カスタムタスクは1ジョブで回す
	//
	// 読み書きシグネチャが衝突するタスク同士は、後ろのタスクの直前(同期)か
	// 後続として積む(Job)ことで待ち合わせる。フェーズの終わりでは全ジョブを待つ。
	//
	// シグネチャに出てこない依存(コリジョンワールドへの submit、デバッグ描画、
	// オーディオ、構造変更の予約、リソース等)は拾えないので、Job にするタスクは
	// 宣言したコンポーネント以外に触らないこと(オプトイン)
	//==========================================================================================
	class SystemManager
	{
	public:

		// 初期化
		void Init();

		//----------------------------------------------------------------------------------
		// システム実体の寿命を預かる
		//
		// タスクの登録(Init)は上位層が済ませてから渡す。基盤はシステムが
		// 何を引数に取るかを知らないので、生成と初期化には関与しない。
		//----------------------------------------------------------------------------------
		void Hold(std::shared_ptr<ISystem> a_spSystem);

		// システムの更新
		// システムのフェーズを指定、コンテキストを入れる。
		// プロファイラを渡したときだけタスクごとの時間を計って渡す(計測のみで実行には影響しない)
		void RunSystem(
			const ESystemType& a_type, const SystemContext& a_context, ECSWorldProfiler* a_pProfiler = nullptr
		);

		// 登録されたタスクをフェーズごとにソートする
		void Sort();

		// タスクの登録
		void AddSystemTask(
			ESystemType a_systemType,const SystemTask& a_systemTask,const std::string& a_taskName
		);

		// システムの型ごとのIDを取得 : 保存されることはないからランタイムのみ
		template<typename T>
		static uint32_t GetID()
		{
			static uint32_t _id = s_systemCounter++;
			return _id;
		}

	private:

		// ソート失敗(依存の循環)時に、巻き込まれたタスクをログへ出して
		// 登録順で末尾へ足す。黙って実行されなくなるのを防ぐための後始末。
		void ReportSortFailure(
			ESystemType a_phase,
			const std::vector<SystemTask*>& a_allTaskVec,
			std::vector<SystemTask*>& a_sortedTaskVec
		);

		// 並びの診断を作る : 循環に巻き込まれたものと、前後が依存で決まっていない衝突を集める
		void BuildScheduleReport(
			ESystemType a_phase,
			const std::vector<SystemTask*>& a_allTaskVec,
			const std::vector<SystemTask*>& a_sortedTaskVec,
			size_t a_sortedCount
		);

	public:

		// ---- アクセサ ----
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap() const;

		// 実行時の待ち合わせまで組んだもの(待つ相手の並び位置を持つ)
		const std::unordered_map<ESystemType, std::vector<CompileTask>>& GetCompiledTaskMap() const { return m_compiledTaskMap; }

		// 並びの診断
		const std::unordered_map<ESystemType, PhaseScheduleReport>& GetScheduleReportMap() const { return m_scheduleReportMap; }

	private:

		inline static uint32_t s_systemCounter = 0;

		// 登録されているシステム実体(寿命の保持のみ)
		std::vector<std::shared_ptr<ISystem>> m_systemVec;

		// 登録されているタスク
		//
		// ソート結果は SystemTask* で持つので、実体のアドレスが動くと壊れる。
		// 遅延初期化のシステムが実行中にタスクを積む経路があり、
		// vector<SystemTask> のままだと push_back の再確保で
		// ソート済みのポインタがぶら下がるため、実体を個別に確保する
		std::unordered_map<ESystemType, std::vector<std::unique_ptr<SystemTask>>> m_systemTaskMap = {};

		// ソート後のタスク
		std::unordered_map<ESystemType, std::vector<SystemTask*>> m_compileTaskMap = {};

		// コンパイル済みタスク
		std::unordered_map<ESystemType, std::vector<CompileTask>> m_compiledTaskMap = {};

		// 並びの診断(Sort のたびに作り直す)
		std::unordered_map<ESystemType, PhaseScheduleReport> m_scheduleReportMap = {};

		// 変更があるかどうか
		bool m_isChange = false;

		// ランタイム中メンバ : RunSystem 1回の間だけ有効
		std::vector<Thread::Job*> m_jobScratch;				// 並び位置 → いまフレームのジョブ
		std::vector<Thread::Job*> m_depScratch;				// 1タスク分の待つ相手

		std::vector<Thread::Job*> m_batchScratch;			// １タスクを分割した際のジョブたち

		// 並び位置 → 計測時間(ns)
		// 分割したタスクは複数のワーカーが同時に足し込むのでアトミックで持つ。
		// アトミックはムーブできず vector の assign / resize が使えないため配列で持ち、
		// 足りないときだけ RunSystem の先頭(ジョブが1つも走っていない時点)で作り直す
		std::unique_ptr<std::atomic<int64_t>[]> m_upTaskNsScratch = nullptr;
		uint32_t m_taskNsCapacity = 0;
	};

}
