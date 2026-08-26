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

	//==========================================================================================
	// システムの管理
	//
	// システムはメインスレッドで、ソートされた順に1つずつ実行される。
	// 並列化したいシステムは、そのシステムの中でジョブを発行して
	// 自分の終わりで待ち合わせること。
	// システム同士を並列に回すのは、読み書きシグネチャに出てこない依存
	// (コリジョンワールドへの submit、エディタのデバッグ描画、オーディオ等)を
	// 拾えないため行わない
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

	private:

		// ソート失敗(依存の循環)時に、巻き込まれたタスクをログへ出して
		// 登録順で末尾へ足す。黙って実行されなくなるのを防ぐための後始末。
		void ReportSortFailure(
			ESystemType a_phase,
			const std::vector<SystemTask*>& a_allTaskVec,
			std::vector<SystemTask*>& a_sortedTaskVec
		);

	public:

		// ---- アクセサ ----
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap() const;

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

		// ソート後のタスク
		std::unordered_map<ESystemType, std::vector<SystemTask*>> m_compileTaskMap = {};

		// 変更があるかどうか
		bool m_isChange = false;
	};

}
