#pragma once

#include "../../System/SystemCommon.h"
#include "../../Resource/ResourceTypeManager.h"

namespace Engine::ECS
{
	class World;
	class SystemManager;
	struct SystemTask;

	//==========================================================================================
	// 計測結果(スナップショット)
	//
	// Capture した時点のワールドの状態を値で写したもの。
	// ワールドの中身へのポインタは持たない(チャンクのアドレスは表示用の値として持つだけ)ので、
	// 取った後にワールドが変わっても、消えても読める
	//==========================================================================================

	// エンティティ
	struct ECSEntityProfile
	{
		uint32_t	aliveCount = 0;		// 生存数
		size_t		slotCount = 0;		// 確保済みの枠数(未使用含む)
		size_t		recycleCount = 0;	// 再利用待ちの枠数
	};

	// チャンクのメモリ(ChunkAllocator)
	struct ECSChunkMemoryProfile
	{
		size_t blockCount = 0;			// 確保済みブロック数
		size_t blockChunkNum = 0;		// 1ブロックあたりのチャンク数
		size_t totalChunkCount = 0;		// 確保済みチャンク数
		size_t usedChunkCount = 0;		// 貸し出し中のチャンク数
		size_t freeChunkCount = 0;		// 貸し出していないチャンク数
		size_t chunkBytes = 0;			// 1チャンクのバイト数
		size_t reservedBytes = 0;		// 確保済みのバイト数(全ブロック)
		size_t liveBytes = 0;			// 生きているエンティティが実際に使っているバイト数
	};

	// アーキタイプ内のコンポーネント配列の配置
	struct ECSComponentLayoutProfile
	{
		ComponentTypeID	typeID = 0;
		std::string		name = {};
		size_t			size = 0;		// sizeof
		size_t			align = 0;		// alignof
		size_t			offset = 0;		// チャンク先頭からの位置
		size_t			stride = 0;		// 要素の間隔
	};

	// チャンク1つ
	struct ECSChunkProfile
	{
		uintptr_t	address = 0;		// 表示用(触らないこと)
		uint32_t	count = 0;			// 入っているエンティティ数
		bool		isFreeChunk = false;// 空きとして手元に残しているチャンクか
	};

	// アーキタイプ1つ
	struct ECSArchetypeProfile
	{
		size_t									index = 0;			// 生成順
		Signature								signature = {};
		std::vector<ECSComponentLayoutProfile>	components = {};	// 配置順(オフセット順)
		uint32_t								chunkCapacity = 0;	// 1チャンクに入る最大数
		size_t									maxAlign = 0;		// コンポーネントの最大アライメント
		size_t									entityStride = 0;	// 1エンティティのバイト数(エンティティ配列込み)
		size_t									layoutBytes = 0;	// 満杯時にレイアウトが使うバイト数
		uint32_t								entityCount = 0;	// 全チャンクの合計
		std::vector<ECSChunkProfile>			chunks = {};
	};

	// コンポーネントの型ごとの使われ方
	struct ECSComponentUsageProfile
	{
		ComponentTypeID	typeID = 0;
		std::string		name = {};
		size_t			size = 0;
		size_t			align = 0;
		uint32_t		archetypeCount = 0;	// この型を持つアーキタイプ数
		uint32_t		entityCount = 0;	// この型を持つエンティティ数
	};

	// システムのタスク1つ
	struct ECSSystemTaskProfile
	{
		ESystemType					phase = ESystemType::Num;
		uint32_t					order = 0;				// フェーズ内の実行順
		std::string					name = {};
		std::vector<std::string>	readNames = {};			// 読み込みのコンポーネント
		std::vector<std::string>	writeNames = {};		// 書き込みのコンポーネント

		// 実行のされ方
		bool						isJob = false;			// ワーカーで走るか
		bool						isCyclic = false;		// 循環に巻き込まれ、登録順で末尾に足されたか
		std::vector<std::string>	waitNames = {};			// 実行前に完了を待つ Job タスク
		uint32_t					ambiguityCount = 0;		// 前後が依存で決まっていない衝突の数

		// クエリ(RegisterTask のみ。カスタムタスクは持たない)
		bool						hasQuery = false;
		bool						isQueryStale = false;	// キャッシュが古い(次の実行で作り直される)
		size_t						matchedChunkCount = 0;
		uint32_t					matchedEntityCount = 0;

		// 実行時間(ms) : 計測を始めてからの値
		double						lastMs = 0.0;
		double						averageMs = 0.0;
		double						maxMs = 0.0;
		uint64_t					callCount = 0;
	};

	// 衝突しているのに、依存(RAW)の経路で前後が保証されていない組
	struct ECSScheduleAmbiguityProfile
	{
		std::string					earlierName = {};		// 今の並びで先に走る方
		std::string					laterName = {};			// 今の並びで後に走る方
		std::vector<std::string>	conflictNames = {};		// ぶつかっているコンポーネント
	};

	// フェーズ1つぶんの並びの診断
	struct ECSPhaseScheduleProfile
	{
		ESystemType									phase = ESystemType::Num;
		bool										isSorted = true;		// トポロジカルソートが成功したか
		std::vector<std::string>					cyclicTaskNames = {};	// 循環に巻き込まれたもの
		std::vector<ECSScheduleAmbiguityProfile>	ambiguities = {};
	};

	// リソース1つ
	struct ECSResourceProfile
	{
		ResourceTypeID	id = 0;
		std::string		name = {};
		size_t			size = 0;		// sizeof(ヒープに持っている分は含まない)
	};

	// 構造変更
	struct ECSStructuralChangeProfile
	{
		// 前回の Capture から反映された数
		size_t created = 0;
		size_t changed = 0;
		size_t removed = 0;

		// 今積まれている予約の数
		size_t pendingCreate = 0;
		size_t pendingChange = 0;
		size_t pendingRemove = 0;
		size_t pendingRefresh = 0;
	};

	// ワールド全体
	struct ECSWorldSnapshot
	{
		uint64_t								captureCount = 0;		// 何回目の Capture か(0 なら未取得)
		uint64_t								archetypeGeneration = 0;// アーキタイプの世代
		ECSEntityProfile						entity = {};
		ECSChunkMemoryProfile					memory = {};
		ECSStructuralChangeProfile				structural = {};
		std::vector<ECSArchetypeProfile>		archetypes = {};
		std::vector<ECSComponentUsageProfile>	components = {};		// 添え字がタイプID
		std::vector<ECSSystemTaskProfile>		systemTasks = {};		// フェーズ順 → 実行順
		std::vector<ECSPhaseScheduleProfile>	schedules = {};			// フェーズ順(タスクのあるフェーズだけ)
		std::vector<ECSResourceProfile>			resources = {};
	};

	//==========================================================================================
	// ECSのワールドにオプションでつけれるプロファイラクラス
	//
	// あくまでECSに関することの計測を行うのでデバッグ描画などはエディター側に任せる
	//
	// ワールドは const でしか見ないので、ここからワールドの中身を変えることはできない。
	// 見る側(エディター)は Capture で取り直して GetSnapshot を読むだけ。
	//
	// 実行中に溜める計測値(システムの時間・構造変更の数)は、ワールドとシステムの管理から
	// 呼ばれるフックでのみ受け取る(フックは private で、呼べるのは World / SystemManager だけ)。
	// ワールドがプロファイラを持っていなければフックは呼ばれず、計測の負荷はかからない
	//==========================================================================================
	class ECSWorldProfiler
	{
	public:

		explicit ECSWorldProfiler(const World* a_pOwner);
		~ECSWorldProfiler();

		// コピー禁止(持ち主のワールドに1つ)
		NON_COPYABLE_NON_MOVABLE(ECSWorldProfiler);

		/// <summary>
		/// ワールドの今の状態を取り直す。
		/// 反復(ForEach / システム)の最中やBeginFrameの最中には呼ばないこと
		/// </summary>
		void Capture();

		// 直近の Capture の結果
		const ECSWorldSnapshot& GetSnapshot() const { return m_snapshot; }

		// システムの実行時間の記録を捨てる
		void ResetTaskTimings();

	private:

		//------------------------------------------------------------------------------------------
		// 計測フック
		//------------------------------------------------------------------------------------------
		friend class World;
		friend class SystemManager;

		// タスク1回ぶんの実行時間
		void RecordTaskTime(const SystemTask* a_pTask, double a_ms);

		// 予約の反映
		void RecordCreated(size_t a_num) { m_createdSinceCapture += a_num; }
		void RecordChanged(size_t a_num) { m_changedSinceCapture += a_num; }
		void RecordRemoved(size_t a_num) { m_removedSinceCapture += a_num; }

		//------------------------------------------------------------------------------------------
		// Capture の中身
		//------------------------------------------------------------------------------------------
		void CaptureEntity();
		void CaptureArchetypes();
		void CaptureComponentUsage();
		void CaptureSystemTasks();
		void CaptureSchedules();
		void CaptureResources();
		void CaptureStructuralChange();

		// シグネチャに立っているコンポーネントの名前
		std::vector<std::string> ToComponentNames(const Signature& a_sig) const;

	private:

		// タスクごとの実行時間の積み上げ
		struct TaskTiming
		{
			double		lastMs = 0.0;
			double		averageMs = 0.0;	// 指数移動平均
			double		maxMs = 0.0;
			uint64_t	callCount = 0;
		};

		// 平均の追従の速さ(大きいほど直近の値に寄る)
		static constexpr double TASK_AVERAGE_RATE = 0.05;

		// 計測対象(見るだけ)
		const World* m_pOwner = nullptr;

		// 直近の Capture の結果
		ECSWorldSnapshot m_snapshot = {};

		// タスクごとの実行時間 : タスクの実体は SystemManager が個別に確保していて動かない
		std::unordered_map<const SystemTask*, TaskTiming> m_taskTimingMap = {};

		// 前回の Capture から反映された構造変更の数
		size_t m_createdSinceCapture = 0;
		size_t m_changedSinceCapture = 0;
		size_t m_removedSinceCapture = 0;
	};
}
