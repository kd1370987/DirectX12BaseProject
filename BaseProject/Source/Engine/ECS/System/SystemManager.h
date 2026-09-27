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
		ResourceSignature resReadSig;								// 読み込みのみを行うリソース
		ResourceSignature resWriteSig;								// 書き込みを行うリソース

		// 明示の順序(同じフェーズのタスク名) : 読み書きの依存より優先する
		std::vector<std::string> afterNames;						// このタスクより先に走らせるもの
		std::vector<std::string> beforeNames;						// このタスクより後に走らせるもの

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
	// 並びの決め方
	//
	// フェーズ内の並びは次の2つの辺で決める。
	//   ・明示の順序 : TaskAccess::After / Before で宣言したもの(最優先)
	//   ・RAW        : 読む側を、同じものを書く側の後へ(既定の並べ方)
	// 明示の順序で「読む側が先」と決まっている組では、その RAW は使わない。
	// 読み書きが往復する組(例 : A が X を書いて Y を読み、B が Y を書いて X を読む)は
	// RAW だけでは循環するので、明示の順序を1つ足して向きを決める。
	//
	// 書き手同士や「読んだ後に書く」組み合わせには RAW の辺が張られないので、
	// 明示の順序が無ければ段と登録順で並んでいるにすぎない(= 前後が決まっていない)。
	//------------------------------------------------------------------------------------------

	// [from][to] = from の後に to が走る
	using ScheduleAdjacency = std::vector<std::vector<uint8_t>>;

	//------------------------------------------------------------------------------------------
	// 並びの診断(Sort のたびに作り直す。実行には使わない)
	//------------------------------------------------------------------------------------------

	// 衝突しているのに、順序(明示・RAW)の経路で前後が保証されていない組
	struct ScheduleAmbiguity
	{
		const SystemTask* pEarlier = nullptr;	// 今の並びで先に走る方
		const SystemTask* pLater = nullptr;		// 今の並びで後に走る方
		Signature conflictSig;					// ぶつかっているコンポーネント
		ResourceSignature resConflictSig;		// ぶつかっているリソース
	};

	// フェーズ1つぶんの診断
	struct PhaseScheduleReport
	{
		bool isSorted = true;								// トポロジカルソートが成功したか
		std::vector<const SystemTask*> cyclicTaskVec;		// 循環に巻き込まれ、登録順で末尾に足されたもの
		std::vector<ScheduleAmbiguity> ambiguityVec;		// 前後が依存で決まっていない衝突

		// 明示の順序で打ち消した RAW(読む側, 書く側)。読む側が先に走る
		std::vector<std::pair<const SystemTask*, const SystemTask*>> overriddenRawVec;

		// 同じフェーズに見つからなかった順序の宣言(「タスク名 -> After(相手)」の形)
		std::vector<std::string> unknownOrderVec;
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

		// タスクの登録 : 登録したタスクの実体を返す(依存の追加宣言に使う。アドレスは動かない)
		SystemTask* AddSystemTask(
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

		// 並べ方の元になるグラフを組む : 明示の順序 + RAW(明示で逆向きが決まっている組は除く)。
		// 打ち消した RAW と、見つからなかった順序の宣言は a_report へ残す
		void BuildPhaseGraph(
			const std::vector<SystemTask*>& a_taskVec,
			ScheduleAdjacency& a_outAdj,
			PhaseScheduleReport& a_report
		);

		// ソート失敗(依存の循環)時に、巻き込まれたタスクをログへ出して
		// 登録順で末尾へ足す。黙って実行されなくなるのを防ぐための後始末。
		void ReportSortFailure(
			ESystemType a_phase,
			const std::vector<SystemTask*>& a_allTaskVec,
			std::vector<SystemTask*>& a_sortedTaskVec,
			const ScheduleAdjacency& a_adj
		);

		// 並びの診断を作る : 循環に巻き込まれたものと、前後が決まっていない衝突を集める
		void BuildScheduleReport(
			ESystemType a_phase,
			const std::vector<SystemTask*>& a_allTaskVec,
			const std::vector<SystemTask*>& a_sortedTaskVec,
			size_t a_sortedCount,
			const ScheduleAdjacency& a_adj,
			PhaseScheduleReport& a_report
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
